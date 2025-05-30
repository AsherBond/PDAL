/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include <mutex>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

#include <gdal.h>
#include <gdal_priv.h>
#include <ogr_api.h>
#include <ogr_geometry.h>
#include <ogrsf_frmts.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <nlohmann/json.hpp>
#include <pdal/Polygon.hpp>
#include <pdal/private/OGRSpec.hpp>
#include <pdal/private/SrsTransform.hpp>
#include <pdal/private/gdal/SpatialRef.hpp>

#include "GDALUtils.hpp"

namespace pdal
{

namespace gdal
{

/**
  Reproject a point from a source projection to a destination.
  \param x  X coordinate of point to be reprojected in-place.
  \param y  Y coordinate of point to be reprojected in-place.
  \param z  Z coordinate of point to be reprojected in-place.
  \param srcSrs  Source SRS
  \param dstSrs  Destination SRS
  \return  Whether the reprojection was successful or not.
*/
bool reproject(double& x, double& y, double& z, const SpatialReference& srcSrs,
    const SpatialReference& dstSrs)
{
    return SrsTransform(srcSrs, dstSrs).transform(x, y, z);
}


/**
  Reproject a bounds box from a source projection to a destination.
  \param box  Bounds box to be reprojected in-place.
  \param srcSrs  Source SRS.
  \param dstSrs  Destination SRS.
  \return  Whether the reprojection was successful or not.
*/
bool reprojectBounds(BOX3D& box, const SpatialReference& srcSrs,
    const SpatialReference& dstSrs)
{
    SrsTransform transform(srcSrs, dstSrs);

    bool ok = transform.transform(box.minx, box.miny, box.minz);
    if (ok)
        ok = transform.transform(box.maxx, box.maxy, box.maxz);
    return ok;
}


/**
  Reproject a bounds box from a source projection to a destination.
  \param box  2D or 3D bounds box to be reprojected.
  \param srcSrs  Source SRS.
  \param dstSrs  Destination SRS.
  \return  Whether the reprojection was successful or not.
*/
bool reprojectBounds(Bounds& box, const SpatialReference& srcSrs,
    const SpatialReference& dstSrs)
{
    bool ok = false;
    if (box.is3d())
    {
        BOX3D b3 = box.to3d();
        ok = reprojectBounds(b3, srcSrs, dstSrs);
        box.reset(b3);
    }
    else
    {
        BOX2D b2 = box.to2d();
        ok = reprojectBounds(b2, srcSrs, dstSrs);
        box.reset(b2);
    }
    return ok;
}

/**
  Reproject a bounds box from a source projection to a destination.
  \param box  2D Bounds box to be reprojected in-place.
  \param srcSrs  Source SRS.
  \param dstSrs  Destination SRS.
  \return  Whether the reprojection was successful or not.
*/
bool reprojectBounds(BOX2D& box, const SpatialReference& srcSrs,
    const SpatialReference& dstSrs)
{
    BOX3D b(box);
    bool res = reprojectBounds(b, srcSrs, dstSrs);
    box = b.to2d();
    return res;
}


/**
  Get the last error from a GDAL/OGR operation as a string.
  \return  Error message.
*/
std::string lastError()
{
    return CPLGetLastErrorMsg();
}


/**
  Register GDAL/OGR drivers.
*/
void registerDrivers()
{
    static std::once_flag flag;

    auto init = []() -> void
    {
        GDALAllRegister();
        OGRRegisterAll();
    };

    std::call_once(flag, init);
}


/**
  Unregister GDAL/OGR drivers.
*/
void unregisterDrivers()
{
    GDALDestroyDriverManager();
}


/**
  Create OGR geometry given a well-known text string and text SRS.
  \param s  WKT string to convert to OGR Geometry.
  \param srs  Text representation of coordinate reference system.
  \return  Pointer to new geometry.
*/
OGRGeometry *createFromWkt(const std::string& s, std::string& srs)
{
    OGRGeometry *newGeom;
	char *buf = const_cast<char *>(s.data());
    OGRGeometryFactory::createFromWkt(&buf, nullptr, &newGeom);
    if (!newGeom)
        throw pdal_error("Couldn't convert WKT string to geometry.");
    srs = buf;

	std::string::size_type pos = 0;
	pos = Utils::extractSpaces(srs, pos);
	if (pos == srs.size())
		srs.clear();
    else
    {
        if (srs[pos++] != '/')
            throw pdal_error("Invalid character following valid geometry.");
        pos += Utils::extractSpaces(srs, pos);
        srs = srs.substr(pos);
    }

    return newGeom;
}

/**
  Create OGR geometry given a wkb string and text SRS.
  \param s  WKT string to convert to OGR Geometry.
  \param srs  Text representation of coordinate reference system.
  \return  Pointer to new geometry.
*/
OGRGeometry *createFromWkb(const std::string& s, std::string& srs)
{
    OGRGeometry *newGeom(nullptr);

    size_t nBytesRead;
    OGRErr err = OGRGeometryFactory::createFromWkb(s.c_str(),
                                                   NULL,
                                                   &newGeom,
                                                   s.size(),
                                                   wkbVariantIso,
                                                   nBytesRead);
    if (!newGeom)
        throw pdal_error("Couldn't convert WKB string to geometry.");

    return newGeom;
}


/**
  Create OGR geometry given a GEOjson text string and text SRS.
  \param s  GEOjson string to convert to OGR Geometry.
  \param srs  Text representation of coordinate reference system.
  \return  Pointer to new geometry.
*/
OGRGeometry *createFromGeoJson(const std::string& s, std::string& srs)
{
    NL::json root;

    std::string json(s);
    json.erase(std::remove(json.begin(), json.end(), '\\'), json.end());
    try
    {
        root = NL::json::parse(json, /* callback */ nullptr,
                                  /* allow exceptions */ true,
                                  /* ignore_comments */ false);
    }
    catch (NL::json::parse_error& err)
    {
        // Look for a right bracket -- this indicates the start of the
        // actual message from the parse error.
        std::string s(err.what());
        auto pos = s.find("]");
        if (pos != std::string::npos)
            s = s.substr(pos + 1);
        std::stringstream msg;

        msg << "Failed to parse GeoJSON with error: " <<  s;
        throw pdal_error(msg.str());
    }


    std::string text;
    text = root.dump();
    OGRGeometry *newGeom = OGRGeometryFactory::createFromGeoJson(text.c_str());
    if (!newGeom)
        throw pdal_error("Couldn't convert GeoJSON to geometry.");

    if (root.contains("crs") || root.contains("srs"))
    {
        NL::json node;

        // We have an 'srs' object
        if (root.contains("crs"))
            node = root.at("crs");
        if (root.contains("srs"))
            node = root.at("srs");

        if (node.is_string())
            srs = node.get<std::string>();
        else
            throw pdal_error ("'srs' or 'crs' node was not a string");

        srs.erase(std::remove(srs.begin(), srs.end(), '\\'), srs.end());
    }

    return newGeom;
}


/**
  Load polygons from an OGR datasource specified by JSON.
  \param ogr  JSON that specifies how to load data.
  \return  Vector of polygons read from datasource.
*/
std::vector<Polygon> getPolygons(const OGRSpecOptions& ogr)
{
    registerDrivers();

    char** papszDriverOptions = nullptr;
    for (const auto& s: ogr.drivers)
        papszDriverOptions = CSLAddString(papszDriverOptions, s.c_str());

    char** papszOpenOptions = nullptr;
    for(const auto& s: ogr.openOpts)
        papszOpenOptions = CSLAddString(papszOpenOptions, s.c_str());

    unsigned int openFlags =
        GDAL_OF_READONLY | GDAL_OF_VECTOR | GDAL_OF_VERBOSE_ERROR;
    GDALDataset* ds;
    ds = (GDALDataset*) GDALOpenEx(ogr.datasource.c_str(), openFlags,
        papszDriverOptions, papszOpenOptions, NULL);
    CSLDestroy(papszDriverOptions);
    CSLDestroy(papszOpenOptions);
    if (!ds)
        throw pdal_error("Unable to read OGR datasource: " + ogr.datasource);

    OGRLayer* poLayer(nullptr);
    if (!ogr.layer.empty())
    {
        poLayer = ds->GetLayerByName( ogr.layer.c_str() );

        if (!poLayer)
            throw pdal_error("Unable to read OGR layer: " + ogr.layer);
    } else
    {
        int nLayer = ds->GetLayerCount();
        if (nLayer < 1)
        {
            throw pdal_error("No layers available on datasource " + ogr.datasource);
        }
        // We just grab the first layer
        poLayer = ds->GetLayer(0);
    }

    OGRFeature *poFeature (nullptr);
    if (!ogr.sql.empty())
    {
        std::string dialect("OGRSQL");
        if (!ogr.dialect.empty())
            dialect = ogr.dialect;

        std::string query = ogr.sql;

        Polygon poly;
        OGRGeometry *geom = nullptr;
        if (!ogr.geometry.empty())
        {
            // Determine the layer's SRS and assign it to the geometry
            // or transform to that SRS.
            poLayer = ds->ExecuteSQL(query.c_str(), NULL, dialect.c_str());
            if (!poLayer)
                throw pdal_error("Unable to execute OGR SQL query.");

            SpatialRef sref;
            sref.setFromLayer(reinterpret_cast<OGRLayerH>(poLayer));
            ds->ReleaseResultSet(poLayer);

            poly.update(ogr.geometry);
            if (poly.srsValid())
            {
                auto ok = poly.transform(sref.wkt());
                if (!ok)
                    throw pdal_error(ok.what());
            }
            else
                poly.setSpatialReference(sref.wkt());

            geom = (OGRGeometry *)poly.getOGRHandle();
        }
        poLayer = ds->ExecuteSQL(query.c_str(), geom, dialect.c_str());
        if (!poLayer)
            throw pdal_error("unable to execute sql query!");
    }

    std::vector<Polygon> polys;
    while ((poFeature = poLayer->GetNextFeature()) != NULL)
    {
        polys.emplace_back(reinterpret_cast<OGRGeometryH>(poFeature->GetGeometryRef()));
        OGRFeature::DestroyFeature( poFeature );
    }

    // if we used a SQL filter, we need to release the
    // dataset
    if (!ogr.sql.empty())
    {
        ds->ReleaseResultSet(poLayer);
    }
    return polys;
}


} // namespace gdal
} // namespace pdal

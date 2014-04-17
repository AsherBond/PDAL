/******************************************************************************
* Copyright (c) 2012-2014, Bradley J Chambers (brad.chambers@gmail.com)
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
*
* Inspired, and partially borrowed from VTK_PCL_Conversions
* https://github.com/daviddoria/VTK_PCL_Conversions
****************************************************************************/

#ifndef INCLUDED_PCL_CONVERSIONS_HPP
#define INCLUDED_PCL_CONVERSIONS_HPP

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>

#include <pdal/Dimension.hpp>
#include <pdal/FileUtils.hpp>
#include <pdal/Options.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/Reader.hpp>
#include <pdal/Schema.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/StageIterator.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/for_each_type.h>
#include <pcl/point_types.h>
#include <pcl/point_traits.h>

namespace pdal
{
/**
 * \brief Convert PCD point cloud to PDAL.
 *
 * Converts PCD data to PDAL format.
 */
template <typename CloudT>
void PCDtoPDAL(CloudT &cloud, PointBuffer& data)
{
    typedef typename pcl::traits::fieldList<typename CloudT::PointType>::type FieldList;

    const pdal::Schema &buffer_schema = data.getSchema();
    data.setNumPoints(cloud.points.size());

    // Begin by setting XYZ dimension data
    const pdal::Dimension &dX = buffer_schema.getDimension("X");
    const pdal::Dimension &dY = buffer_schema.getDimension("Y");
    const pdal::Dimension &dZ = buffer_schema.getDimension("Z");
    if (pcl::traits::has_xyz<typename CloudT::PointType>::value)
    {
        for (boost::uint32_t i = 0; i < cloud.points.size(); ++i)
        {
            typename CloudT::PointType p = cloud.points[i];
            data.setField<float>(dX, i, p.x);
            data.setField<float>(dY, i, p.y);
            data.setField<float>(dZ, i, p.z);
        }
    }

    // Continue with optional Intensity dimension
    boost::optional<pdal::Dimension const &> dI = buffer_schema.getDimensionOptional("Intensity");
    if (dI && pcl::traits::has_intensity<typename CloudT::PointType>::value)
    {
        for (boost::uint32_t i = 0; i < cloud.points.size(); ++i)
        {
            typename CloudT::PointType p = cloud.points[i];
            float v;
            pcl::for_each_type<FieldList> (
                pcl::CopyIfFieldExists<typename CloudT::PointType, float> (
                    p, "intensity", v
                )
            );
            data.setField<float>(*dI, i, v);
        }
    }

    // Conclude with optional RGB dimensions
    boost::optional<pdal::Dimension const &> dR = buffer_schema.getDimensionOptional("Red");
    boost::optional<pdal::Dimension const &> dG = buffer_schema.getDimensionOptional("Green");
    boost::optional<pdal::Dimension const &> dB = buffer_schema.getDimensionOptional("Blue");
    if (dR && dG && dB && pcl::traits::has_color<typename CloudT::PointType>::value)
    {
        for (boost::uint32_t i = 0; i < cloud.points.size(); ++i)
        {
            typename CloudT::PointType p = cloud.points[i];
            boost::uint32_t v;
            pcl::for_each_type<FieldList> (
                pcl::CopyIfFieldExists<typename CloudT::PointType, boost::uint32_t> (
                    p, "rgba", v
                )
            );
            data.setField<boost::uint8_t>(*dR, i, (v & 0x00FF0000) >> 16);
            data.setField<boost::uint8_t>(*dG, i, (v & 0x0000FF00) >> 8);
            data.setField<boost::uint8_t>(*dB, i, (v & 0x000000FF));
        }
    }
}

/**
 * \brief Convert PDAL point cloud to PCD.
 *
 * Converts PDAL data to PCD format.
 */
template <typename CloudT>
void PDALtoPCD(const PointBuffer& data, CloudT &cloud)
{
    typedef typename pcl::traits::fieldList<typename CloudT::PointType>::type FieldList;

    const pdal::Schema &buffer_schema = data.getSchema();

    cloud.width = data.getNumPoints();
    cloud.height = 1;  // unorganized point cloud
    cloud.is_dense = false;
    cloud.points.resize(cloud.width);

    // Begin by getting XYZ dimension data
    const pdal::Dimension &dX = buffer_schema.getDimension("X");
    const pdal::Dimension &dY = buffer_schema.getDimension("Y");
    const pdal::Dimension &dZ = buffer_schema.getDimension("Z");
    if (pcl::traits::has_xyz<typename CloudT::PointType>::value)
    {
      std::cerr << std::setprecision(4);
      std::cerr << std::fixed;
      std::cerr << data.getFieldAs<float>(dX, 0, true) << std::endl;
      std::cerr << data.getFieldAs<float>(dX, 0, false) << std::endl;
      std::cerr << data.getFieldAs<float>(dX, 0, false)*dX.getNumericScale() << std::endl;
      std::cerr << data.getFieldAs<float>(dX, 0, false)*dX.getNumericScale()+dX.getNumericOffset() << std::endl;
      std::cerr << data.getFieldAs<int32_t>(dX, 0, false)*dX.getNumericScale()+dX.getNumericOffset() << std::endl;
        for (boost::uint32_t i = 0; i < cloud.points.size(); ++i)
        {
            typename CloudT::PointType p = cloud.points[i];
            p.x = data.getFieldAs<float>(dX, i);
            p.y = data.getFieldAs<float>(dY, i);
            p.z = data.getFieldAs<float>(dZ, i);
            cloud.points[i] = p;
        }
    }

    // Continue with optional Intensity dimension
    boost::optional<pdal::Dimension const &> dI = buffer_schema.getDimensionOptional("Intensity");
    if (dI && pcl::traits::has_intensity<typename CloudT::PointType>::value)
    {
        for (boost::uint32_t i = 0; i < cloud.points.size(); ++i)
        {
            typename CloudT::PointType p = cloud.points[i];
            pcl::for_each_type<FieldList> (
                pcl::SetIfFieldExists<typename CloudT::PointType, float> (
                    p, "intensity", data.getFieldAs<float>(*dI, i)
                )
            );
            cloud.points[i] = p;
        }
    }

    // Conclude with optional RGB dimensions
    boost::optional<pdal::Dimension const &> dR = buffer_schema.getDimensionOptional("Red");
    boost::optional<pdal::Dimension const &> dG = buffer_schema.getDimensionOptional("Green");
    boost::optional<pdal::Dimension const &> dB = buffer_schema.getDimensionOptional("Blue");
    if (dR && dG && dB && pcl::traits::has_color<typename CloudT::PointType>::value)
    {
        for (boost::uint32_t i = 0; i < cloud.points.size(); ++i)
        {
            typename CloudT::PointType p = cloud.points[i];
            boost::uint8_t r = data.getFieldAs<boost::uint8_t>(*dR, i);
            boost::uint8_t g = data.getFieldAs<boost::uint8_t>(*dG, i);
            boost::uint8_t b = data.getFieldAs<boost::uint8_t>(*dB, i);
            pcl::for_each_type<FieldList> (
                pcl::SetIfFieldExists<typename CloudT::PointType, boost::uint32_t> (
                    p, "rgba", ((int)r) << 16 | ((int)g) << 8 | ((int)b)
                )
            );
            cloud.points[i] = p;
        }
    }
}
}  // pdal

#endif  // INCLUDED_PCL_CONVERSIONS_HPP


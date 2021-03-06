/******************************************************************************
* Copyright (c) 2013, Howard Butler (hobu.inc@gmail.com)
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

#ifndef INCLUDED_PDAL_KERNEL_DELTA_HPP
#define INCLUDED_PDAL_KERNEL_DELTA_HPP

#include <pdal/Stage.hpp>
#include <pdal/StageIterator.hpp>
#include <pdal/FileUtils.hpp>
#include <pdal/PointBuffer.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/density.hpp>

#include "Application.hpp"

namespace pdal { namespace kernel {
    

typedef boost::accumulators::accumulator_set<double, boost::accumulators::features<     boost::accumulators::droppable<boost::accumulators::tag::mean>,
        boost::accumulators::droppable<boost::accumulators::tag::max>,
        boost::accumulators::droppable<boost::accumulators::tag::min>,
        boost::accumulators::droppable<boost::accumulators::tag::count> > > summary_accumulator;

class PDAL_DLL Point
{
public:
    double x;
    double y;
    double z;
    boost::uint64_t id;
    
    Point(double x, double y, double z, uint64_t id=0)
        : x(x), y(y),z(z), id(id) {};
    Point(Point const& other) : x(other.x), y(other.y), z(other.z), id(other.id) {};
    Point() : x(0.0), y(0.0), z(0.0), id(0) {};
    bool equal(Point const& other) const
    {
        return (Utils::compare_distance(x, other.x) && 
                Utils::compare_distance(y, other.y) && 
                Utils::compare_distance(z, other.z));
        
    }
    bool operator==(Point const& other) const
    {
        return equal(other);
    }
    bool operator!=(Point const& other) const
    {
        return !equal(other);
    }
    bool operator<(Point const& other) const
    {
        return id < other.id;
    }       
};

class PDAL_DLL Delta : public Application
{
public:
    Delta(int argc, const char* argv[]);
    int execute(); // overrride
    
    
private:
    void addSwitches(); // overrride
    void validateSwitches(); // overrride
    
    std::string m_sourceFile;
    std::string m_candidateFile;
    std::string m_wkt;

    std::ostream* m_outputStream;
    std::string m_outputFileName;
    
    summary_accumulator m_summary_x;
    summary_accumulator m_summary_y;
    summary_accumulator m_summary_z;
	
    bool m_3d;
    bool m_OutputDetail;
    bool m_useXML;
    bool m_useJSON;
    
    void outputRST(boost::property_tree::ptree const&) const;
    void outputXML(boost::property_tree::ptree const&) const;
    void outputJSON(boost::property_tree::ptree const&) const;
    void outputDetail(PointBuffer& source_data,
                         IndexedPointBuffer& candidate_data,
                         std::map<Point, Point> *points) const;

};

}} // pdal::kernel

#endif
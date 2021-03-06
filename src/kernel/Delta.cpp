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

#include <pdal/kernel/Delta.hpp>
#include <boost/format.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace pdal { namespace kernel {
    
Delta::Delta(int argc, const char* argv[])
    : Application(argc, argv, "delta")
    , m_sourceFile("")
    , m_candidateFile("")
    , m_outputStream(0)
    , m_outputFileName("")
    , m_3d(true)
    , m_OutputDetail(false)
    , m_useXML(false)
    , m_useJSON(false)
{
    return;
}


void Delta::validateSwitches()
{
    m_chunkSize = 1048576;     

    return;
}


void Delta::addSwitches()
{
    namespace po = boost::program_options;

    po::options_description* file_options = new po::options_description("file options");
    

    file_options->add_options()
        ("source", po::value<std::string>(&m_sourceFile), "source file name")
        ("candidate", po::value<std::string>(&m_candidateFile), "candidate file name")
        ("output", po::value<std::string>(&m_outputFileName), "output file name")
        ("2d", po::value<bool>(&m_3d)->zero_tokens()->implicit_value(false), "only 2D comparisons/indexing")
        ("detail", po::value<bool>(&m_OutputDetail)->zero_tokens()->implicit_value(true), "Output deltas per-point")
        ("output", po::value<std::string>(&m_outputFileName), "output file name")
        ("xml", po::value<bool>(&m_useXML)->zero_tokens()->implicit_value(true), "dump XML")
        ("json", po::value<bool>(&m_useJSON)->zero_tokens()->implicit_value(true), "dump JSON")

        ;

    addSwitchSet(file_options);

    po::options_description* processing_options = new po::options_description("processing options");
    
    processing_options->add_options()

        ;
    
    addSwitchSet(processing_options);

    addPositionalSwitch("source", 1);
    addPositionalSwitch("candidate", 2);
    addPositionalSwitch("output", 3);

    return;
}



std::ostream& writeHeader(std::ostream& strm, bool b3D)
{
    strm << "\"ID\",\"DeltaX\",\"DeltaY\"";
    if (b3D)
        strm << ",\"DeltaZ\"";
    strm << std::endl;
    return strm;
    
}

std::map<Point, Point>* cumulatePoints(PointBuffer& source_data,
                  IndexedPointBuffer& candidate_data)
{
    std::map<Point, Point> *output = new std::map<Point, Point>;
    boost::uint32_t count(std::min(source_data.getNumPoints(), candidate_data.getNumPoints()));
    

    Schema const& candidate_schema = candidate_data.getSchema();
    Dimension const& cDimX = candidate_schema.getDimension("X");
    Dimension const& cDimY = candidate_schema.getDimension("Y");
    Dimension const& cDimZ = candidate_schema.getDimension("Z");

    Schema const& source_schema = source_data.getSchema();
    Dimension const& sDimX = source_schema.getDimension("X");
    Dimension const& sDimY = source_schema.getDimension("Y");
    Dimension const& sDimZ = source_schema.getDimension("Z");    
    for (boost::uint32_t i = 0; i < count; ++i)
    {
        double sx = source_data.applyScaling(sDimX, i);
        double sy = source_data.applyScaling(sDimY, i);
        double sz = source_data.applyScaling(sDimZ, i);                
        
        std::vector<std::size_t> ids = candidate_data.neighbors(sx, sy, sz, 1);
        
        if (!ids.size())
        {
			std::ostringstream oss;
			oss << "unable to find point for id '"  << i <<"'";
            throw app_runtime_error(oss.str() );
		}
        
        std::size_t id = ids[0];
        double cx = candidate_data.applyScaling(cDimX, id);
        double cy = candidate_data.applyScaling(cDimY, id);
        double cz = candidate_data.applyScaling(cDimZ, id);
        
        Point s(sx, sy, sz, i);
        Point c(cx, cy, cz, id);
        output->insert(std::pair<Point, Point>(s, c));

        double xd = sx - cx;
        double yd = sy - cy;
        double zd = sz - cz;
    }
  
    return output;
}

void Delta::outputDetail(PointBuffer& source_data,
                         IndexedPointBuffer& candidate_data,
                         std::map<Point, Point> *points) const
{
    Schema const& candidate_schema = candidate_data.getSchema();
    Dimension const& cDimX = candidate_schema.getDimension("X");
    Dimension const& cDimY = candidate_schema.getDimension("Y");
    Dimension const& cDimZ = candidate_schema.getDimension("Z");

    Schema const& source_schema = source_data.getSchema();
    Dimension const& sDimX = source_schema.getDimension("X");
    Dimension const& sDimY = source_schema.getDimension("Y");
    Dimension const& sDimZ = source_schema.getDimension("Z");
    
    bool bWroteHeader(false);
    
    std::ostream& ostr = m_outputStream ? *m_outputStream : std::cout;
    
    candidate_data.build(m_3d);
    boost::uint32_t count(std::min(source_data.getNumPoints(), candidate_data.getNumPoints()));
    
    boost::property_tree::ptree output;
    for (boost::uint32_t i = 0; i < count; ++i)
    {
        double sx = source_data.applyScaling(sDimX, i);
        double sy = source_data.applyScaling(sDimY, i);
        double sz = source_data.applyScaling(sDimZ, i);                
        
        std::vector<std::size_t> ids = candidate_data.neighbors(sx, sy, sz, 1);
        
        if (!ids.size())
        {
			std::ostringstream oss;
			oss << "unable to find point for id '"  << i <<"'";
            throw app_runtime_error(oss.str() );
		}
        
        std::size_t id = ids[0];
        double cx = candidate_data.applyScaling(cDimX, id);
        double cy = candidate_data.applyScaling(cDimY, id);
        double cz = candidate_data.applyScaling(cDimZ, id);
        
        Point s(sx, sy, sz, id);
        Point c(cx, cy, cz, id);
        
        double xd = sx - cx;
        double yd = sy - cy;
        double zd = sz - cz;
        boost::property_tree::ptree pt;
        pt.put<boost::int32_t>("i", i);
        pt.put<float>("xd", xd);
        pt.put<float>("yd", yd);
        if (m_3d)        
            pt.put<float>("zd", zd);
        
        output.add_child("delta", pt);

    }

    if (m_useXML)
    {
        boost::property_tree::write_xml(ostr, output);
    } else if (m_useJSON)
    {
        boost::property_tree::write_json(ostr, output);
        
    } else
    {
        writeHeader(ostr, m_3d);

        for (auto b = output.begin(); b != output.end(); ++b)
        {
            ostr << b->second.get<boost::int32_t>("i")  << ",";
            boost::uint32_t precision = Utils::getStreamPrecision(cDimX.getNumericScale());
            ostr.setf(std::ios_base::fixed, std::ios_base::floatfield);
            ostr.precision(precision);
            ostr << b->second.get<float>("xd") << ",";
  
            precision = Utils::getStreamPrecision(cDimY.getNumericScale());
            ostr.precision(precision);
            ostr << b->second.get<float>("yd");
    
            if (m_3d)
            {
                ostr << ",";
                precision = Utils::getStreamPrecision(cDimZ.getNumericScale());
                ostr.precision(precision);
                ostr << b->second.get<float>("zd");
            }
        }
   
    
        ostr << std::endl;        
    }




    if (m_outputStream)
    {
        FileUtils::closeFile(m_outputStream);
    }    
}

void Delta::outputRST(boost::property_tree::ptree const& tree) const
{
    std::string headline("------------------------------------------------------------------------------------------");
    std::cout << headline << std::endl;
    std::cout << " Delta summary for source '" << m_sourceFile << "' and candidate '" << m_candidateFile <<"'" << std::endl;
    std::cout << headline << std::endl;
    std::cout << std::endl;
    
    std::string thead("----------- --------------- --------------- --------------");
    std::cout << thead << std::endl;
    std::cout << " Dimension       X             Y                  Z    " << std::endl;
    std::cout << thead << std::endl;
    
    boost::format fmt("%.4f");
 
    
    
    std::cout << " Min        " << fmt % tree.get<float>("min.x") << "            " << fmt % tree.get<float>("min.y") << "            " << fmt % tree.get<float>("min.z")<<std::endl;
    std::cout << " Min        " << fmt % tree.get<float>("max.x") << "            " << fmt % tree.get<float>("max.y") << "            " << fmt % tree.get<float>("max.z")<<std::endl;
    std::cout << " Mean       " << fmt % tree.get<float>("mean.x") << "            " << fmt % tree.get<float>("mean.y") << "            " << fmt % tree.get<float>("mean.z")<<std::endl;
    std::cout << thead << std::endl;
    
}


void Delta::outputJSON(boost::property_tree::ptree const& tree) const
{
    boost::property_tree::write_json(std::cout, tree);
    
}

void Delta::outputXML(boost::property_tree::ptree const& tree) const
{
    boost::property_tree::write_xml(std::cout, tree);
    
}

int Delta::execute()
{

    Options sourceOptions;
    {
        sourceOptions.add<std::string>("filename", m_sourceFile);
        sourceOptions.add<bool>("debug", isDebug());
        sourceOptions.add<boost::uint32_t>("verbose", getVerboseLevel());
    }
    Stage* source = AppSupport::makeReader(sourceOptions);
    source->initialize();
    
    boost::uint32_t totalPointCount(source->getNumPoints());
    
    PointBuffer source_data(source->getSchema(), totalPointCount);
    StageSequentialIterator* source_iter = source->createSequentialIterator(source_data);

    boost::uint32_t  numRead = source_iter->read(source_data);
    assert(numRead == source_data.getNumPoints());

    delete source_iter;
    delete source;



    Options candidateOptions;
    {
        candidateOptions.add<std::string>("filename", m_candidateFile);
        candidateOptions.add<bool>("debug", isDebug());
        candidateOptions.add<boost::uint32_t>("verbose", getVerboseLevel());
    }

    Stage* candidate = AppSupport::makeReader(candidateOptions);

    candidate->initialize();    


    IndexedPointBuffer candidate_data(candidate->getSchema(), totalPointCount);
    StageSequentialIterator* candidate_iter = candidate->createSequentialIterator(candidate_data);

    numRead = candidate_iter->read(candidate_data);
    assert(numRead == candidate_data.getNumPoints());
        
    delete candidate_iter;    


    if (source_data.getNumPoints() != candidate_data.getNumPoints())
    {
        std::cerr << "Source and candidate files do not have the same point count, testing each source point only!" << std::endl;
    }
    

    if (m_outputFileName.size())
    {
        m_outputStream = FileUtils::createFile(m_outputFileName);
    }

    candidate_data.build(m_3d);
    boost::uint32_t count(std::min(source_data.getNumPoints(), candidate_data.getNumPoints()));
    


    boost::scoped_ptr<std::map<Point, Point> > points(cumulatePoints(source_data, candidate_data));
    if (m_OutputDetail)
    {
        outputDetail(source_data, candidate_data, points.get());
        return 0;
    }
    
    std::map<Point, Point>::const_iterator i;
    for(i = points->begin(); i != points->end(); ++i)
    {
        Point const& s = i->first;
        Point const& c = i->second;

        double xd = s.x - c.x;
        double yd = s.y - c.y;
        double zd = s.z - c.z;        
        m_summary_x(xd);
        m_summary_y(yd);
        m_summary_z(zd);        
    }

    using boost::property_tree::ptree;
    ptree output;

    double sminx  = (boost::accumulators::min)(m_summary_x);
    double sminy  = (boost::accumulators::min)(m_summary_y);
    double sminz  = (boost::accumulators::min)(m_summary_z);
    double smaxx  = (boost::accumulators::max)(m_summary_x);
    double smaxy  = (boost::accumulators::max)(m_summary_y);
    double smaxz  = (boost::accumulators::max)(m_summary_z);
    
    double smeanx  = (boost::accumulators::mean)(m_summary_x);
    double smeany  = (boost::accumulators::mean)(m_summary_y);
    double smeanz  = (boost::accumulators::mean)(m_summary_z);

    output.put<float>("min.x", sminx);
    output.put<float>("min.y", sminy);
    output.put<float>("min.z", sminz);
    output.put<float>("max.x", smaxx);
    output.put<float>("max.y", smaxy);
    output.put<float>("max.z", smaxz);
    output.put<float>("mean.x", smeanx);
    output.put<float>("mean.y", smeany);
    output.put<float>("mean.z", smeanz);
    output.put<std::string>("source", m_sourceFile);
    output.put<std::string>("candidate", m_candidateFile);
    
    
    if (m_useJSON)
    {
        outputJSON(output);
        return 0;
    } else if (m_useXML)
    {
        outputXML(output);
        return 0;
    }
    else
    {
        outputRST(output);
        return 0;
    }

    
    return 0;
}

}} // pdal::kernel

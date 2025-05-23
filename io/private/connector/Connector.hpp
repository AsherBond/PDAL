/******************************************************************************
 * Copyright (c) 2018, Connor Manning
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
 *     * Neither the name of the Martin Isenburg or Iowa Department
 *       of Natural Resources nor the names of its contributors may be
 *       used to endorse or promote products derived from this software
 *       without specific prior written permission.
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

#pragma once

#include <arbiter/arbiter.hpp>
#include <pdal/FileSpec.hpp>

using StringMap = std::map<std::string, std::string>;

namespace pdal
{
namespace connector
{

class Connector
{
    std::unique_ptr<arbiter::Arbiter> m_arbiter;
    StringMap m_headers;
    StringMap m_query;
    std::unique_ptr<arbiter::drivers::Http> m_httpDriver;
    std::string m_filename;

public:
    Connector();
    Connector(const std::string& filename, const StringMap& headers, const StringMap& query);
    Connector(const StringMap& headers, const StringMap& query);
    Connector(const FileSpec& spec);

    std::string get(const std::string& path) const;
    NL::json getJson(const std::string& path) const;
    std::vector<char> getBinary(const std::string& path) const;
    arbiter::LocalHandle getLocalHandle(const std::string& path) const;
    void put(const std::string& path, const std::vector<char>& data) const;
    void put(const std::string& path, const std::string& data) const;
    void makeDir(const std::string& path) const;
    StringMap headRequest(const std::string& path) const;

    std::vector<char> getBinary(uint64_t offset, int32_t size) const;
};

} // namespace ept
} // namespace pdal


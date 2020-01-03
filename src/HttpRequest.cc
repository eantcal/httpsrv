//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "HttpRequest.h"


/* -------------------------------------------------------------------------- */

void HttpRequest::parseMethod(const std::string& method)
{
    if (method == "GET")
        _method = Method::GET;
    else if (method == "HEAD")
        _method = Method::HEAD;
    else if (method == "POST")
        _method = Method::POST;
    else
        _method = Method::UNKNOWN;
}


/* -------------------------------------------------------------------------- */

void HttpRequest::parseVersion(const std::string& ver)
{
    const size_t vstrlen = sizeof("HTTP/x.x") - 1;
    std::string v = ver.size() > vstrlen ? ver.substr(0, vstrlen) : ver;

    if (v == "HTTP/1.0")
        _version = Version::HTTP_1_0;
    else if (v == "HTTP/1.1")
        _version = Version::HTTP_1_1;
    else
        _version = Version::UNKNOWN;
}


/* -------------------------------------------------------------------------- */

std::ostream& HttpRequest::dump(std::ostream& os, const std::string& id)
{
    std::string ss;
    
    ss = ">>> REQUEST " + id + "\n";

    for (auto e : get_header())
        ss += e;
    
    os << ss << "\n";

    return os;
}


/* -------------------------------------------------------------------------- */

void HttpRequest::parseHeader(const std::string& new_header) 
{
    if (new_header.empty())
        return;

    const auto prefix = ::toupper(new_header.c_str()[0]);

    if (prefix == 'C' || prefix == 'E') {
        std::vector<std::string> tokens;

        Tools::splitLineInTokens(new_header, tokens, " ");

        const auto headerName = Tools::uppercase(tokens[0]);

        if (tokens.size() >= 2) {
            if (prefix == 'C') {
                if (headerName == "CONTENT-LENGTH:") {
                    try {
                        _content_length = std::stoi(tokens[1]);
                    }
                    catch (...) {
                        _content_length = 0;
                    }
                }
                else if (headerName == "CONTENT-TYPE:") {
                    _content_type = tokens[1];

                    std::cerr << "Content-type:" << _content_type << std::endl;
                }
                else if (headerName == "CONTENT-DISPOSITION:") {
                    std::vector<std::string> htoken;
                    Tools::splitLineInTokens(new_header, htoken, ";");

                    if (!htoken.empty()) {
                        const std::string searched_prefix = "filename=\"";

                        for (const auto& item : htoken) {
                            if (item.size() > searched_prefix.size() &&
                                item.substr(0, searched_prefix.size()) == searched_prefix)
                            {
                                for (auto i = searched_prefix.size(); i < item.size(); ++i) {
                                    if (item[i] == '\"')
                                        break;
                                    _filename += item[i];
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else {
                if (headerName == "EXPECT:" && tokens[1][0]=='1') {
                    const auto value = Tools::uppercase(Tools::trim(tokens[1]));
                    _expected_100_continue = value == "100-CONTINUE";

                    std::cerr << "*********** 100 **********\n";
                }
            }
        }
    }
    _header.push_back(new_header);
}
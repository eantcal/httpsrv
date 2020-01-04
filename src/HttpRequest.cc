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

    for (auto e : getHeaderList())
        ss += e;
    
    os << ss << "\n";

    return os;
}


/* -------------------------------------------------------------------------- */

void HttpRequest::parseHeader(const std::string& header) 
{
    const auto prefix = ::toupper(header.c_str()[0]);

    if (prefix == 'C' || prefix == 'E') {
        std::vector<std::string> tokens;

        Tools::splitLineInTokens(header, tokens, " ");

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
                    std::vector<std::string> content_type;
                    Tools::splitLineInTokens(header, content_type, ";");
                    if (!content_type.empty()) {
                        const std::string searched_prefix = "boundary=";

                        for (const auto& item : content_type) {
                            auto field = Tools::trim(item);
                            if (field.size() > searched_prefix.size() &&
                                field.substr(0, searched_prefix.size()) == searched_prefix)
                            {
                                _boundary = field.substr(
                                    searched_prefix.size(), field.size() - searched_prefix.size());
                                break;
                            }
                        }
                    }
                }
                else if (headerName == "CONTENT-DISPOSITION:") {
                    std::vector<std::string> htoken;
                    Tools::splitLineInTokens(header, htoken, ";");

                    if (!htoken.empty()) {
                        const std::string searched_prefix = "filename=\"";

                        for (const auto& item : htoken) {
                            auto field = Tools::trim(item);
                            if (field.size() > searched_prefix.size() &&
                                field.substr(0, searched_prefix.size()) == searched_prefix)
                            {
                                _filename.clear();
                                for (auto i = searched_prefix.size(); i < field.size(); ++i) {
                                    if (field[i] == '\"')
                                        break;
                                    _filename += field[i];
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
                }
            }
        }
    }
}
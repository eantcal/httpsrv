//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__


/* -------------------------------------------------------------------------- */
#include "Tools.h"

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>


/* -------------------------------------------------------------------------- */

/**
 * Encapsulates HTTP style request, consisting of a request line,
 * some headers, and a content body
 */
class HttpRequest {
public:
    using Handle = std::shared_ptr<HttpRequest>;

    enum class Method { GET, HEAD, POST, UNKNOWN };
    enum class Version { HTTP_1_0, HTTP_1_1, UNKNOWN };

    HttpRequest() = default;
    HttpRequest(const HttpRequest&) = default;


    /**
     * Returns the request headers
     */
    const std::list<std::string>& get_header() const { 
       return _header; 
    }


    /**
     * Returns the method of the command line (GET, HEAD, ...)
     */
    const Method& getMethod() const noexcept { 
       return _method; 
    }


    /**
     * Returns the HTTP version (HTTP/1.0, HTTP/1.1, ...)
     */
    const Version& getVersion() const noexcept { 
       return _version; 
    }


    /**
     * Returns the command line URI
     */
    const std::string& getUri() const noexcept { 
       return _uri; 
    }


    /**
     * Parses the HTTP method.
     *
     * @param method input string
     */
    void parseMethod(const std::string& method);


    /**
     * Parses the URI field.
     *
     * @param uri The input string to parse
     */
    void parseUri(const std::string& uri) {
        _uri = uri == "/" ? "index.html" : uri;
    }


    /**
     * Parses the HTTP version.
     *
     * @param ver The input string to parse
     */
    void parseVersion(const std::string& ver);


    /**
     * Adds a new header to request
     *
     * @param new_header The header content to add to headers fields
     */
    void addHeader(const std::string& new_header) {
        if (!new_header.empty() && ::toupper(new_header.c_str()[0])=='C') {
            std::vector<std::string> tokens;

            Tools::splitLineInTokens(new_header, tokens, " ");

            transform(
                tokens[0].begin(), 
                tokens[0].end(), 
                tokens[0].begin(), 
                ::toupper);

            if (tokens.size()>=2 && tokens[0] == "CONTENT-LENGTH:") {
                try {
                    _content_length = std::stoi(tokens[1]);
                }
                catch (...) {
                    _content_length = 0;
                }
            }
        }
        _header.push_back(new_header);
    }

    /**
     * Get the content length field value
     *
     * @return content length in bytes
     */
    int getContentLength() const noexcept {
        return _content_length;
    }

    /**
     * Set a body to request
     *
     * @param new_header The header content to add to headers fields
     */
    void setBody(std::string&& body) {
        _body = std::move(body);
    }

    /**
     * Prints the request.
     *
     * @param os output stream
     * @param id request identifier
     * @return output stream reference
     */
    std::ostream& dump(std::ostream& os, const std::string& id = "");


private:
    std::list<std::string> _header;
    Method _method = Method::UNKNOWN;
    Version _version = Version::UNKNOWN;
    std::string _uri;
    std::string _body;
    int _content_length = 0;
};


/* -------------------------------------------------------------------------- */

#endif // __HTTP_REQUEST_H__

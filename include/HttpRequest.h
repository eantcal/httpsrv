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
#include "StrUtils.h"

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "config.h"


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

   using HeaderList = std::list<std::string>;


   /**
    * Returns the request headers
    */
   const HeaderList& getHeaderList() const {
      return _headerList;
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

   // TODO
   void setUri(const std::string& uri) {
      _uri = uri;
   }

   /**
    * Returns the command line URI args (arg1/arg2/.../argN)
    */
   const std::vector<std::string>& getUriArgs() const noexcept {
      return _uriArgs;
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
      StrUtils::splitLineInTokens(StrUtils::trim(uri), _uriArgs, "/");
      _uri = uri; // == "/" ? "index.html" : uri;
   }


   /**
    * Parses the HTTP version.
    *
    * @param ver The input string to parse
    */
   void parseVersion(const std::string& ver);


   /**
    * Parse line as part of request header
    *
    * @param header The header content to add to headers fields
    */
   void parseHeader(const std::string& header);


   /**
    * Adds a new line to request data
    *
    * @param line to add
    */
   void addLine(const std::string& line) {
      _headerList.push_back(line);
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
    * Return true if the request contains "Expect: 100-continue"
    *
    * @return true if continue request sent by client, false otherwise
    */
   bool isExpectedContinueResponse() const noexcept {
      return _expected_100_continue;
   }


   /**
    * Clean "Expect: 100-continue" flag
    */
   void clearExpectedContinueFlag() noexcept {
      _expected_100_continue = false;
   }


   /**
    * Get the content-disposition fileName attribute content
    *
    * @return string containing any file name posted
    */
   const std::string& getFileName() const noexcept {
      return _filename;
   }


   /**
    * Set fileName
    */
   void setFileName(const std::string& fileName) noexcept {
      _filename = fileName;
   }


   /**
    * Get the content-type multipart boundary
    *
    * @return string containing any boundary string
    */
   const std::string& getBoundary() const noexcept {
      return _boundary;
   }


   /**
    * Set a body to request
    *
    * @param body is body content
    */
   void setBody(std::string&& body) {
      _body = std::move(body);
   }

   /**
    * Get any body
    * @param body is body content
    */
   const std::string& getBody() const noexcept {
      return _body;
   }


   /**
    * Prints the request.
    *
    * @param os output stream
    * @param id request identifier
    * @return output stream reference
    */
   std::ostream& dump(std::ostream& os, const std::string& id = "");

   /**
    * Return true if the GET request is valid, false otherwise
    */
   bool isValidGetRequest() const noexcept {
      return 
         (getMethod() == HttpRequest::Method::GET && 
          (getUri() == HTTP_SERVER_GET_MRUFILES ||
           getUri() == HTTP_SERVER_GET_FILES || 
           (getUriArgs().size() == 3 && 
            getUriArgs()[1]== HTTP_URIPFX_FILES) ||
           (getUriArgs().size() == 4 && 
            getUriArgs()[1]== HTTP_URIPFX_FILES &&
            getUriArgs()[3]== HTTP_URISFX_ZIP)));
   }

   bool isValidPostRequest() const noexcept {
      return getMethod() == HttpRequest::Method::POST
         && getUri() == HTTP_SERVER_POST_STORE
         && !isExpectedContinueResponse()
         && !getFileName().empty();
   }


private:
   HeaderList _headerList;
   Method _method = Method::UNKNOWN;
   Version _version = Version::UNKNOWN;
   std::string _uri;
   std::string _body;
   int _content_length = 0;
   std::string _content_type;
   std::string _filename;
   std::string _boundary;
   bool _expected_100_continue = false;
   std::vector<std::string> _uriArgs;
};


/* -------------------------------------------------------------------------- */

#endif // __HTTP_REQUEST_H__

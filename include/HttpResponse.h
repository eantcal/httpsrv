//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__


/* -------------------------------------------------------------------------- */

#include "HttpRequest.h"

#include <unordered_map>
#include <string>


/* -------------------------------------------------------------------------- */

/**
 * Encapsulates HTTP style response, consisting of a status line,
 * some headers, and a content body.
 */
class HttpResponse {
public:
   HttpResponse() = delete;
   HttpResponse(const HttpResponse&) = default;
   HttpResponse& operator=(const HttpResponse&) = default;

   using Handle = std::unique_ptr<HttpResponse>;

   /**
    * Constructs a response to a request.
    *
    * @param request an http request
    * @param webRootPath local working directory of the web server
    * @param body optional body field
    */
   HttpResponse(
      const HttpRequest& request,
      const std::string& webRootPath,
      const std::string& fileName,
      const std::string& body = "",
      const std::string& bodyFormat = "");


   /**
    * Returns the content of response status line and response headers.
    */
   operator const std::string& () const {
      return _response;
   }


   /**
    * Returns the content of local resource related to the URI requested.
    */
   const std::string& getLocalUriPath() const {
      return _localUriPath;
   }


   /**
    * Returns the content of local resource repository root path
    */
   const std::string& getLocalRepositoryPath() const {
      return _localRepositoryPath;
   }


   /**
    * Prints the response out to os stream.
    *
    * @param os The output stream
    * @param id A string used to identify the response
    * @return the os output stream
    */
   std::ostream& dump(std::ostream& os, const std::string& id = "");

private:
   static std::unordered_map<std::string, std::string> _mimeTbl;

   std::string _response;
   std::string _localUriPath;
   std::string _localRepositoryPath;

   // Format an error response
   static void formatError(
      std::string& output,
      int code,
      const std::string& msg);

   // Format an positive response
   static void formatPositiveResponse(
      std::string& response,
      const std::string& fileTime,
      const std::string& fileExt,
      const size_t& contentLen);
public:
   // Format an positive response
   static void formatContinueResponse(
      std::string& response);
};


/* -------------------------------------------------------------------------- */

#endif // __HTTP_RESPONSE_H__

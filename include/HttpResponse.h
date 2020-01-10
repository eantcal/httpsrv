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
#include <cassert>
#include <memory>

/* -------------------------------------------------------------------------- */

/**
 * Encapsulates HTTP response, consisting of a status line,
 * some headers, and a content body.
 */
class HttpResponse
{
public:
   HttpResponse() = delete;
   HttpResponse(const HttpResponse &) = default;
   HttpResponse &operator=(const HttpResponse &) = default;

   using Handle = std::unique_ptr<HttpResponse>;

   /**
    * Constructs a response to a given request.
    * @param request is the request
    * @param body is optional body content
    * @param bodyFormat is optional body format
    * @param fileToSend is optional file name to send
    */
   HttpResponse(
       const HttpRequest &request,
       const std::string &body,
       const std::string &bodyFormat,
       const std::string &fileToSend);

   /**
    * Constructs an error response depending on given errorCode.
    */
   HttpResponse(int errorCode)
   {
      formatError(errorCode);
   }

   /**
    * Returns the content of response status line and response headers.
    */
   operator const std::string& () const
   {
      return _response;
   }

   /**
    * Writes response into output stream.
    *
    * @param os a given output stream
    * @param id a string used to identify the response
    * @return the os output stream
    */
   std::ostream &dump(std::ostream &os, const std::string &id = "");

   /**
    * Returns ture if HTTP error code is 4xx/5xx,
    * false otherwise
    */
   bool isErrorResponse() const throw()
   {
      return _errorResponse;
   }

private:
   static std::unordered_map<std::string, std::string> _mimeTbl;
   static std::unordered_map<int, std::string> _errTbl;

   std::string _response;
   bool _errorResponse = false;

   // Format an error response
   void formatError(int code);

   // Format an positive response
   void formatPositiveResponse(
       const std::string &fileTime,
       const std::string &fileExt,
       const size_t &contentLen);

   // Format an positive response
   void formatContinueResponse();
};

/* -------------------------------------------------------------------------- */

#endif // __HTTP_RESPONSE_H__

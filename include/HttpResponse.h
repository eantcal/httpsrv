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
    */
   HttpResponse(
      const HttpRequest& request,
      const std::string& body,
      const std::string& bodyFormat,
      const std::string& fileToSend);


   /**
    * Returns the content of response status line and response headers.
    */
   operator const std::string& () const {
      return _response;
   }


   /**
    * Prints the response out to os stream.
    *
    * @param os The output stream
    * @param id A string used to identify the response
    * @return the os output stream
    */
   std::ostream& dump(std::ostream& os, const std::string& id = "");

   // TODO
   bool isErrorResponse() const throw() {
      return _errorResponse;
   }

private:
   static std::unordered_map<std::string, std::string> _mimeTbl;

   std::string _response;
   bool _errorResponse = false;

   // Format an error response
   void formatError(int code, const std::string& msg);

   // Format an positive response
   void formatPositiveResponse(
      const std::string& fileTime,
      const std::string& fileExt,
      const size_t& contentLen);

   // Format an positive response
   void formatContinueResponse();
};


/* -------------------------------------------------------------------------- */

#endif // __HTTP_RESPONSE_H__

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

void HttpRequest::parseMethod(const std::string &method)
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

void HttpRequest::parseVersion(const std::string &ver)
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

std::ostream &HttpRequest::dump(std::ostream &os, const std::string &id)
{
   std::string ss;

   ss = ">>> REQUEST " + id + "\n";

   for (auto e : getHeaderList())
      ss += e;

   os << ss << std::endl;

   return os;
}

/* -------------------------------------------------------------------------- */

void HttpRequest::parseHeader(const std::string &header)
{
   const auto prefix = ::toupper(header.c_str()[0]);

   if (prefix == 'C' || prefix == 'E')
   {
      std::vector<std::string> tokens;

      StrUtils::splitLineInTokens(header, tokens, " ");

      const auto headerName = StrUtils::uppercase(tokens[0]);

      if (tokens.size() >= 2)
      {
         if (prefix == 'C')
         {
            // Parse the Content-Length header
            if (headerName == "CONTENT-LENGTH:")
            {
               try
               {
                  _contentLength = std::stoi(tokens[1]);
               }
               catch (...)
               {
                  _contentLength = 0;
               }
            }
            // Parse the content type searching for the boundary marker
            // which looks like as in the following example:
            //
            // Content-Type: multipart/form-data; boundary=-----490a4289f7afa3e5
            //
            else if (headerName == "CONTENT-TYPE:")
            {
               _contentType = tokens[1];
               std::vector<std::string> content_type;
               StrUtils::splitLineInTokens(header, content_type, ";");
               if (!content_type.empty())
               {
                  const std::string searched_prefix = "boundary=";

                  for (const auto &item : content_type)
                  {
                     auto field = StrUtils::trim(item);
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
            //  Parse multipart content and search for content disposition:
            //  From such header get the filename field used to store file content
            //  Such header looks like in the following example:
            //
            //  Content-Disposition: form-data; name="file"; filename="File02.txt"
            //
            else if (headerName == "CONTENT-DISPOSITION:")
            {
               std::vector<std::string> htoken;
               StrUtils::splitLineInTokens(header, htoken, ";");

               if (!htoken.empty())
               {
                  const std::string searched_prefix = "filename=\"";

                  for (const auto &item : htoken)
                  {
                     auto field = StrUtils::trim(item);
                     if (field.size() > searched_prefix.size() &&
                         field.substr(0, searched_prefix.size()) == searched_prefix)
                     {
                        _filename.clear();
                        for (auto i = searched_prefix.size(); i < field.size(); ++i)
                        {
                           const bool escape = field[i-1]=='\\';

                           /*
                           Escape punctuation characters:
                          
                           Even if RFC6266 - Appendix D reccomends not to use them
                           CURL and other clients might use escape in quoted string,
                           so we try to fix it by removing such useless characters (here)

                           \" = quotation mark (backslash not required for '"')
                           \' = apostrophe (backslash not required for "'")
                           \? = question mark (used to avoid trigraphs)
                           \\ = backslash
                           */
                           const auto ch=field[i];
                           const bool punctuation = escape && (ch=='\"' || ch=='\'' || ch=='\?' || ch=='\\'); 

                           if (escape && punctuation && !_filename.empty()) {
                              _filename.resize(_filename.size()-1); // remove last backslash
                              _filename += ch; // replace it with punctuation
                           }
                           else if (ch == '\"')
                              break;
                           else   
                              _filename += ch;
                        }

                        break;
                     }
                  }
               }
            }
         }
         // Parse the Expect header, to identify the request to 
         // send 100-Continue to the client in order to get the
         // rest of multi-part header/body
         else
         {
            if (headerName == "EXPECT:" && tokens[1][0] == '1')
            {
               const auto value = StrUtils::uppercase(StrUtils::trim(tokens[1]));
               _expected_100_continue = value == "100-CONTINUE";
            }
         }
      }
   }
}

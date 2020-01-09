//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "HttpSocket.h"
#include "StrUtils.h"
#include "SysUtils.h"

#include <vector>
#include <thread>

/* -------------------------------------------------------------------------- */

HttpSocket &HttpSocket::operator=(TcpSocket::Handle handle)
{
   _socketHandle = handle;
   return *this;
}

/* -------------------------------------------------------------------------- */

bool HttpSocket::recv(HttpRequest::Handle &handle)
{
   char c = 0;
   int ret = 1;

   enum class CrLfSeq
   {
      CR1,
      LF1,
      CR2,
      LF2,
      IDLE
   } crlf_st = CrLfSeq::IDLE;

   auto feed_crlf_fsm = [&crlf_st](char c) {
      switch (crlf_st)
      {
      case CrLfSeq::IDLE:
         crlf_st = (c == '\r') ? CrLfSeq::CR1 : CrLfSeq::IDLE;
         break;
      case CrLfSeq::CR1:
         crlf_st = (c == '\n') ? CrLfSeq::LF1 : CrLfSeq::IDLE;
         break;
      case CrLfSeq::LF1:
         crlf_st = (c == '\r') ? CrLfSeq::CR2 : CrLfSeq::IDLE;
         break;
      case CrLfSeq::CR2:
         crlf_st = (c == '\n') ? CrLfSeq::LF2 : CrLfSeq::IDLE;
         break;
      default:
         crlf_st = CrLfSeq::IDLE;
         break;
      }
   };

   std::string line;
   std::string body;

   bool receivingBody = false;
   bool boundary_maker = false;
   bool timeout = false;

   while (ret > 0 && _connUp && _socketHandle)
   {
      std::chrono::milliseconds msec(getConnectionTimeout());

      auto recvEv = _socketHandle->waitForRecvEvent(msec);

      switch (recvEv)
      {
      case TransportSocket::RecvEvent::RECV_ERROR:
         _connUp = false;
         break;
      case TransportSocket::RecvEvent::TIMEOUT:
         timeout = true;
         break;
      default:
         break;
      }

      if (!_connUp || timeout)
         break;

      ret = _socketHandle->recv(&c, 1);

      if (ret > 0)
      {
         line += c;
      }
      else if (ret <= 0)
      {
         _connUp = false;
         break;
      }

      feed_crlf_fsm(c);

      if (!receivingBody && crlf_st == CrLfSeq::LF2)
      {
         if (line == "\r\n")
            line.clear();

         if (!handle->isExpectedContinueResponse())
         {
            receivingBody = handle->getBoundary().empty() || boundary_maker;
         }
         else
            break;
      }

      if ((crlf_st == CrLfSeq::LF1 || crlf_st == CrLfSeq::LF2) && !line.empty())
      {
         if (!handle->getBoundary().empty())
         {
            const std::string boundary_begin = "--" + handle->getBoundary();
            const std::string boundary_end = "--" + handle->getBoundary() + "--";

            const std::string trimmedLine = StrUtils::trim(line);

            if (!receivingBody && !boundary_maker && trimmedLine == boundary_begin)
            {
               boundary_maker = true;
            }
            else if (receivingBody && boundary_maker && trimmedLine == boundary_end)
            {
               receivingBody = false;
               line.clear();
            }
         }

         if (!line.empty())
         {
            if (!receivingBody)
            {
               handle->parseHeader(line);
               handle->addLine(line);
            }
            else
            {
               body += line;
            }

            line.clear();
         }
      }
   }

   if (ret < 0 || !_socketHandle || handle->getHeaderList().empty())
   {
      return false;
   }

   std::string request = *handle->getHeaderList().cbegin();
   std::vector<std::string> tokens;

   if (!StrUtils::splitLineInTokens(request, tokens, " "))
   {
      return false;
   }

   if (tokens.size() != 3)
   {
      return false;
   }

   handle->parseMethod(tokens[0]);
   handle->parseUri(tokens[1]);
   handle->parseVersion(tokens[2]);

   //in case the body is encapsulated in boundary markers a prior CRLF sequence
   //that was already added to the body should be still considered part of the
   //boundary marker itself rather than part of body. Such CRLF must be removed
   if (boundary_maker && body.size() > 2)
      body.resize(body.size() - 2);

   if (!body.empty())
      handle->setBody(std::move(body));

   return true;
}

/* -------------------------------------------------------------------------- */

HttpSocket &HttpSocket::operator<<(const HttpResponse &response)
{
   const std::string &response_txt = response;
   size_t to_send = response_txt.size();

   while (to_send > 0)
   {
      int sent = _socketHandle->send(response);
      
      if (sent < 0)
      {
         _connUp = false;
         break;
      }

      if (sent == 0)
      {
         // tx queue congested, wait for a while
         std::this_thread::sleep_for(std::chrono::seconds(1));
         continue;
      }

      to_send -= sent;
   }

   return *this;
}

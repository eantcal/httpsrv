//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __HTTP_SERVERSESSION_H__
#define __HTTP_SERVERSESSION_H__

#include "FileRepository.h"
#include "TcpSocket.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <memory>
#include <ostream>

/* -------------------------------------------------------------------------- */
// HttpSession
/* -------------------------------------------------------------------------- */

class HttpSession
{
public:
   using Handle = std::shared_ptr<HttpSession>;

   inline static Handle create(
       bool verboseModeOn,
       std::ostream &loggerOStream,
       TcpSocket::Handle socketHandle,
       FileRepository::Handle FileRepository)
   {
      return Handle(new (std::nothrow) HttpSession(
          verboseModeOn,
          loggerOStream,
          socketHandle,
          FileRepository));
   }

   HttpSession() = delete;
   void operator()(Handle taskHandle);

private:
   bool _verboseModeOn = true;
   std::ostream &_logger;
   TcpSocket::Handle _tcpSocketHandle;
   FileRepository::Handle _FileRepository;
   std::string _sessionId;

   std::ostream &log()
   {
      return _logger;
   }

   TcpSocket::Handle &getTcpSocketHandle()
   {
      return _tcpSocketHandle;
   }

   const std::string &getLocalStorePath() const
   {
      return _FileRepository->getPath();
   }

   HttpSession(
       bool verboseModeOn,
       std::ostream &loggerOStream,
       TcpSocket::Handle socketHandle,
       FileRepository::Handle FileRepository)
       : 
       _verboseModeOn(verboseModeOn), 
       _logger(loggerOStream), 
       _tcpSocketHandle(socketHandle), 
       _FileRepository(FileRepository)
   {
   }

   enum class processAction
   {
      none,
      sendErrorInvalidRequest,
      sendInternalError,
      sendJsonFileList,
      sendMruFiles,
      sendNotFound,
      sendZipFile
   };

   void logSessionBegin();
   void logEnd();

   //! Process HTTP GET Method
   processAction processGetRequest(
       HttpRequest &incomingRequest,
       std::string &json,
       std::string &nameOfFileToSend,
       FileUtils::DirectoryRipper::Handle& zipCleaner);


   //! Process HTTP POST method
   void processPostRequest(
      HttpRequest& incomingRequest, 
      std::string& jsonResponse);
};

/* -------------------------------------------------------------------------- */

#endif // !__HTTP_SERVERSESSION_H__
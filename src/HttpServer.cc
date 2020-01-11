//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */
// HTTP Server

/* -------------------------------------------------------------------------- */

#include "ZipArchive.h"
#include "HttpServer.h"
#include "FileRepository.h"

#include "StrUtils.h"
#include "SysUtils.h"
#include "FileUtils.h"

#include <thread>
#include <cassert>

/* -------------------------------------------------------------------------- */
// HttpServerSession

/* -------------------------------------------------------------------------- */

class HttpServerSession
{
public:
   using Handle = std::shared_ptr<HttpServerSession>;

   inline static Handle create(
       bool verboseModeOn,
       std::ostream &loggerOStream,
       TcpSocket::Handle socketHandle,
       FileRepository::Handle FileRepository)
   {
      return Handle(new (std::nothrow) HttpServerSession(
          verboseModeOn,
          loggerOStream,
          socketHandle,
          FileRepository));
   }

   HttpServerSession() = delete;
   void operator()(Handle taskHandle);

private:
   bool _verboseModeOn = true;
   std::ostream &_logger;
   TcpSocket::Handle _tcpSocketHandle;
   FileRepository::Handle _FileRepository;

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

   HttpServerSession(
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

   enum class ProcessGetRequestResult
   {
      none,
      sendErrorInvalidRequest,
      sendInternalError,
      sendJsonFileList,
      sendMruFiles,
      sendNotFound,
      sendZipFile
   };

   std::string logBegin()
   {
      // Generates an identifier for recognizing HTTP session
      std::string sessionId;

      if (_verboseModeOn)
      {
         const int sd = getTcpSocketHandle()->getSocketFd();
         sessionId =
             "[" + std::to_string(sd) + "] " +
             "[" + SysUtils::getUtcTime() + "] ";

         log() << sessionId << "---- HTTP SERVER SESSION STARTS" << std::endl;
         log().flush();
      }
      return sessionId;
   }

   void logEnd(const std::string &sessionId)
   {
      if (!_verboseModeOn)
         return;

      log() << sessionId << "---- HTTP SERVER SESSION ENDS" << std::endl
            << std::endl;
      log().flush();
   }

   ProcessGetRequestResult processGetRequest(
       HttpRequest &httpRequest,
       std::string &json,
       std::string &fileToSend)
   {
      const auto &uri = httpRequest.getUri();

      // command /files
      if (uri == HTTP_SERVER_GET_FILES &&
         _FileRepository->getFilenameMap().
            locked_updateMakeJson(getLocalStorePath(), json))
      {
         return ProcessGetRequestResult::sendJsonFileList;
      }

      // command /mrufiles
      if (uri == HTTP_SERVER_GET_MRUFILES)
      {
         return !_FileRepository->createJsonMruFilesList(json) ? 
            ProcessGetRequestResult::sendInternalError : 
            ProcessGetRequestResult::sendMruFiles;
      }

      // command /mrufiles/zip
      if (uri == HTTP_SERVER_GET_MRUFILES_ZIP)
      {         
         return _FileRepository->createMruFilesZip(fileToSend) ?
            ProcessGetRequestResult::sendZipFile :
            ProcessGetRequestResult::sendInternalError;
      }

      const auto &uriArgs = httpRequest.getUriArgs();

      // command /files/<id> is split in 3 args (first one, arg[0] is dummy)
      if (uriArgs.size() == 3 && uriArgs[1] == HTTP_URIPFX_FILES)
      {
         const auto &id = httpRequest.getUriArgs()[2];
         if (!_FileRepository->getFilenameMap().
               jsonStatFileUpdateTS(getLocalStorePath(), id, json, true)) 
         { 
            return ProcessGetRequestResult::sendInternalError;
         }
      }

      // command /files/<id>/zip is split in 4 args (first one, arg[0] is dummy)
      if (uriArgs.size() == 4 && 
          uriArgs[1] == HTTP_URIPFX_FILES && 
          uriArgs[3] == HTTP_URISFX_ZIP)
      {
         const auto& id = uriArgs[2];
         auto res = _FileRepository->createFileZip(id, fileToSend);
         switch (res) 
         {
            case FileRepository::createFileZipRes::idNotFound:
               return ProcessGetRequestResult::sendNotFound;

            case FileRepository::createFileZipRes::cantCreateTmpDir:
            case FileRepository::createFileZipRes::cantZipFile:
               return ProcessGetRequestResult::sendInternalError;

            case FileRepository::createFileZipRes::success:
               return ProcessGetRequestResult::sendZipFile;
         }
      }

      return ProcessGetRequestResult::sendErrorInvalidRequest;
   }
};

/* -------------------------------------------------------------------------- */

// Handles the HTTP server request
// This method executes in a specific thread context for
// each accepted HTTP request
void HttpServerSession::operator()(Handle taskHandle)
{
   (void)taskHandle;

   auto sessionId = logBegin();

   // Wait for a request from remote peer
   HttpRequest::Handle httpRequest(new (std::nothrow) HttpRequest);
   assert(httpRequest);
   if (!httpRequest) // out-of-memory?
      return; 

   // Wait for a request from remote peer
   while (getTcpSocketHandle())
   {
      // Create an http socket around a connected tcp socket
      HttpSocket httpSocket(getTcpSocketHandle());
      httpSocket >> httpRequest;

      // If an error occoured terminate the task
      if (!httpSocket)
         break;

      // Log the request
      if (_verboseModeOn)
         httpRequest->dump(log(), sessionId);

      std::string jsonResponse;
      std::string fileToSend;
      ProcessGetRequestResult getRequestAction = ProcessGetRequestResult::none;

      HttpResponse::Handle response;

      if (httpRequest->isExpectedContinueResponse() || 
          httpRequest->isValidPostRequest())
      {
         auto fileName = httpRequest->getFileName();

         if (_verboseModeOn)
         {
            log() << sessionId << "Writing '" << fileName << "'" << std::endl;
            log().flush();
         }

         if (!_FileRepository->store(fileName, httpRequest->getBody(), jsonResponse))
         {
            if (_verboseModeOn)
            {
               log() << sessionId << "Error writing '" << fileName << "'" << std::endl;
               log().flush();
            }
         }
      }
      else if (httpRequest->isValidGetRequest())
      {
         getRequestAction = processGetRequest(*httpRequest, jsonResponse, fileToSend);
      }
      else
      {
         response = std::make_unique<HttpResponse>(400); // Bad Request
      }

      if (!response)
      {
         // Format a response to previous HTTP request, unless a bad
         // request was detected
         response = std::make_unique<HttpResponse>(
             *httpRequest,
             jsonResponse,
             jsonResponse.empty() ? "" : ".json",
             fileToSend);
      }

      assert(response);
      if (!response)
         break;

      // Send the response header and any not empty json content to remote peer
      httpSocket << *response;

      // Any binary content is sent after response header
      if (getRequestAction == ProcessGetRequestResult::sendZipFile)
      {
         if (0 > httpSocket.sendFile(fileToSend))
         {
            if (_verboseModeOn)
            {
               log() << sessionId << "Error sending '" << fileToSend
                     << "'" << std::endl
                     << std::endl;
                     
               log().flush();
            }
            break;
         }
      }

      if (_verboseModeOn)
         response->dump(log(), sessionId);

      if (response->isErrorResponse())
         break;

      if (!httpRequest->isExpectedContinueResponse())
      {
         httpRequest.reset(new (std::nothrow) HttpRequest);
         assert(httpRequest);
         if (!httpRequest)
            break;
      }
      else
      {
         httpRequest->clearExpectedContinueFlag();
      }
   }

   getTcpSocketHandle()->shutdown();

   logEnd(sessionId);
}

/* -------------------------------------------------------------------------- */
// HttpServer

/* -------------------------------------------------------------------------- */

auto HttpServer::getInstance() -> HttpServer &
{
   if (!_instance)
   {
      _instance = new (std::nothrow) HttpServer();
      assert(_instance);
      if (!_instance)
      {
         // no memory, fatal error
         exit(1);
      }
   }

   return *_instance;
}

/* -------------------------------------------------------------------------- */

bool HttpServer::bind(TranspPort port)
{
   _tcpServer = TcpListener::create();
   assert(_tcpServer);

   if (!_tcpServer)
      return false;

   return _tcpServer->bind(port);
}

/* -------------------------------------------------------------------------- */

bool HttpServer::listen(int maxConnections)
{
   if (!_tcpServer)
      return false;

   return _tcpServer->listen(maxConnections);
}

/* -------------------------------------------------------------------------- */

bool HttpServer::run()
{
   // Create a thread for each TCP accepted connection and
   // delegate it to handle HTTP request / response
   while (true)
   {
      const TcpSocket::Handle handle = accept();

      // Accept failed for some reason, retry later
      if (!handle)
      {
         std::this_thread::sleep_for(std::chrono::seconds(1));
         continue;
      }

      assert(_loggerOStreamPtr);

      HttpServerSession::Handle sessionHandle = HttpServerSession::create(
          _verboseModeOn,
          *_loggerOStreamPtr,
          handle,
          _FileRepository);

      // Coping the http_server_task handle (shared_ptr) the reference
      // count is automatically increased by one
      std::thread workerThread(*sessionHandle, sessionHandle);

      workerThread.detach();
   }

   // Ok, following instruction won't be ever executed
   return true;
}

/* -------------------------------------------------------------------------- */

HttpServer *HttpServer::_instance = nullptr;

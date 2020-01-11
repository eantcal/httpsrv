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

   void logSessionBegin()
   {
      if (!_verboseModeOn)
         return;

      const int sd = getTcpSocketHandle()->getSocketFd();
      _sessionId =
         "[" + std::to_string(sd) + "] " +
         "[" + SysUtils::getUtcTime() + "] ";

      log() << _sessionId << "---- HTTP SERVER SESSION STARTS" << std::endl;
      log().flush();
   }

   void logEnd()
   {
      if (!_verboseModeOn)
         return;

      log() << _sessionId << "---- HTTP SERVER SESSION ENDS" << std::endl
            << std::endl;
      log().flush();
   }

   //! Process HTTP GET Method
   processAction processGetRequest(
       HttpRequest &incomingRequest,
       std::string &json,
       std::string &nameOfFileToSend)
   {
      const auto &uri = incomingRequest.getUri();

      // command /files
      if (uri == HTTP_SERVER_GET_FILES &&
         _FileRepository->getFilenameMap().
            locked_updateMakeJson(getLocalStorePath(), json))
      {
         return processAction::sendJsonFileList;
      }

      // command /mrufiles
      if (uri == HTTP_SERVER_GET_MRUFILES)
      {
         return !_FileRepository->createJsonMruFilesList(json) ? 
            processAction::sendInternalError : 
            processAction::sendMruFiles;
      }

      // command /mrufiles/zip
      if (uri == HTTP_SERVER_GET_MRUFILES_ZIP)
      {         
         return _FileRepository->createMruFilesZip(nameOfFileToSend) ?
            processAction::sendZipFile :
            processAction::sendInternalError;
      }

      const auto &uriArgs = incomingRequest.getUriArgs();

      // command /files/<id> is split in 3 args (first one, arg[0] is dummy)
      if (uriArgs.size() == 3 && uriArgs[1] == HTTP_URIPFX_FILES)
      {
         const auto &id = incomingRequest.getUriArgs()[2];
         if (!_FileRepository->getFilenameMap().
               jsonStatFileUpdateTS(getLocalStorePath(), id, json, true)) 
         { 
            return processAction::sendInternalError;
         }
      }

      // command /files/<id>/zip is split in 4 args (first one, arg[0] is dummy)
      if (uriArgs.size() == 4 && 
          uriArgs[1] == HTTP_URIPFX_FILES && 
          uriArgs[3] == HTTP_URISFX_ZIP)
      {
         const auto& id = uriArgs[2];
         auto res = _FileRepository->createFileZip(id, nameOfFileToSend);
         switch (res) 
         {
            case FileRepository::createFileZipRes::idNotFound:
               return processAction::sendNotFound;

            case FileRepository::createFileZipRes::cantCreateTmpDir:
            case FileRepository::createFileZipRes::cantZipFile:
               return processAction::sendInternalError;

            case FileRepository::createFileZipRes::success:
               return processAction::sendZipFile;
         }
      }

      return processAction::sendErrorInvalidRequest;
   }


   //! Process HTTP POST method
   void processPostRequest(HttpRequest& incomingRequest, std::string& jsonResponse) 
   {
      const auto& fileName = incomingRequest.getFileName();

      if (_verboseModeOn)
      {
         log() << _sessionId << "Writing '" << fileName << "'" << std::endl;
         log().flush();
      }

      if (!_FileRepository->store(fileName, incomingRequest.getBody(), jsonResponse))
      {
         if (_verboseModeOn)
         {
            log() << _sessionId << "Error writing '" << fileName << "'" << std::endl;
            log().flush();
         }
      }
   }
};

/* -------------------------------------------------------------------------- */

// Handles the HTTP server request
// This method executes in a specific thread context for
// each accepted HTTP request
void HttpServerSession::operator()(Handle taskHandle)
{
   (void)taskHandle;

   logSessionBegin();

   HttpRequest::Handle incomingRequest(new (std::nothrow) HttpRequest);

   assert(incomingRequest);
   if (!incomingRequest) // out-of-memory?
      return; 

   // Wait for a request from peer
   while (getTcpSocketHandle())
   {
      // Create an http socket around a connected tcp socket
      HttpSocket httpSocket(getTcpSocketHandle());
      httpSocket >> incomingRequest;

      // If an error occoured terminate the task
      if (!httpSocket)
         break;

      // Log the request
      if (_verboseModeOn)
         incomingRequest->dump(log(), _sessionId);

      std::string jsonResponse;
      std::string nameOfFileToSend;
      processAction action = processAction::none;

      HttpResponse::Handle outgoingResponse;

      // if this is a pending POST-request containing 'Expected: 100-Continue'
      if (incomingRequest->isExpected_100_Continue_Response() || 
          // or it is not, then checks if incoming request is
          // a valid POST request 
          incomingRequest->isValidPostRequest())
      {
         processPostRequest(*incomingRequest, jsonResponse);
      }

      // else checks if incomingRequest is valid GET request
      else if (incomingRequest->isValidGetRequest())
      {
         action = processGetRequest(
            *incomingRequest, 
            jsonResponse, 
            nameOfFileToSend);
      }

      // None of above -> respond 400 - Bad Request to the client
      else
      {
         outgoingResponse = std::make_unique<HttpResponse>(400); // Bad Request
      }

      if (!outgoingResponse)
      {
         // Format a response to previous HTTP client request
         outgoingResponse = std::make_unique<HttpResponse>(
             *incomingRequest,
             jsonResponse,
             jsonResponse.empty() ? "" : ".json",
             nameOfFileToSend);
      }

      assert(outgoingResponse);
      if (!outgoingResponse)
         break;

      // Send the response header and any not empty json content to remote peer
      httpSocket << *outgoingResponse;

      // Any binary content is sent following the HTTP response header
      // already sent to the client
      if (action == processAction::sendZipFile)
      {
         if (0 > httpSocket.sendFile(nameOfFileToSend))
         {
            if (_verboseModeOn)
            {
               log() << _sessionId << "Error sending '" << nameOfFileToSend
                     << "'" << std::endl
                     << std::endl;
                     
               log().flush();
            }
            break;
         }
      }

      if (_verboseModeOn)
         outgoingResponse->dump(log(), _sessionId);

      // If we have generated an error, close the session
      if (outgoingResponse->isErrorResponse())
         break;

      if (!incomingRequest->isExpected_100_Continue_Response())
      {
         incomingRequest.reset(new (std::nothrow) HttpRequest);
         assert(incomingRequest);
         if (!incomingRequest)
            break;
      }
      else
      {
         incomingRequest->clearExpectedContinueFlag();
      }
   }

   getTcpSocketHandle()->shutdown();

   logEnd();
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

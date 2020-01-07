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

#include "HttpServer.h"
#include "FileStore.h"

#include "FilenameMap.h"

#include "StrUtils.h"
#include "SysUtils.h"
#include "FileUtils.h"

#include <thread>
#include <cassert>


/* -------------------------------------------------------------------------- */
// HttpServerSession

/* -------------------------------------------------------------------------- */

class HttpServerSession {
public:
   using Handle = std::shared_ptr<HttpServerSession>;

   inline static Handle create(
      bool verboseModeOn,
      std::ostream& loggerOStream,
      TcpSocket::Handle socketHandle,
      FilenameMap::Handle filenameMap,
      FileStore::Handle fileStore
      )
   {
      return Handle(new (std::nothrow) HttpServerSession(
         verboseModeOn,
         loggerOStream,
         socketHandle,
         filenameMap,
         fileStore));
   }

   HttpServerSession() = delete;
   void operator()(Handle task_handle);

private:
   bool _verboseModeOn = true;
   std::ostream& _logger;
   TcpSocket::Handle _tcpSocketHandle;
   FilenameMap::Handle _filenameMap;
   FileStore::Handle _fileStore;

   std::ostream& log() {
      return _logger;
   }

   TcpSocket::Handle& getTcpSocketHandle() {
      return _tcpSocketHandle;
   }

   const std::string& getLocalStorePath() const {
      return _fileStore->getPath();
   }

   HttpServerSession(
      bool verboseModeOn, 
      std::ostream& loggerOStream,
      TcpSocket::Handle socketHandle, 
      FilenameMap::Handle filenameMap,
      FileStore::Handle fileStore)
      : _verboseModeOn(verboseModeOn)
      , _logger(loggerOStream)
      , _tcpSocketHandle(socketHandle)
      , _filenameMap(filenameMap)
      , _fileStore(fileStore)
   {
   }

   bool writePostedFile(
      const std::string& fileName,
      const std::string& fileContent, 
      std::string& json)
   {
      std::string filePath = getLocalStorePath() + "/" + fileName;

      std::ofstream os(filePath, std::ofstream::binary);

      os.write(fileContent.data(), fileContent.size());

      if (!os.fail()) {
         auto id = FileUtils::hashCode(fileName);

         os.close();  // create the file posted by client
         if (!FilenameMap::jsonStat(filePath, fileName, id, json)) {
            json.clear();
         }
         else {
            _filenameMap->insert(id, fileName);
            return true;
         }
      }
      
      return false;
   }

   enum class ProcessGetRequestResult {
      none,
      sendErrorInvalidRequest,
      sendInternalError,
      sendJsonFileList,
      sendNotFound,
      sendZipFile
   };

   ProcessGetRequestResult handleGetReq(
      HttpRequest& httpRequest, 
      std::string& json)
   {
      if (httpRequest.getUri() == HTTP_SERVER_GET_FILES) {
         if (_filenameMap->locked_updateMakeJson(getLocalStorePath(), json)) {
            return ProcessGetRequestResult::sendJsonFileList;
         }
      }
      else if (httpRequest.getUri() == HTTP_SERVER_GET_MRUFILES) {
         FileStore::TimeOrderedFileList timeOrderedFileList;

         if (!_fileStore->createJsonMruFilesList(json)) {
            return ProcessGetRequestResult::sendInternalError;
         }
      }
      else if (httpRequest.getUriArgs().size() == 3 && 
               httpRequest.getUriArgs()[1] == HTTP_URIPFX_FILES) 
      {
         const auto &id = httpRequest.getUriArgs()[2];
         if (!_filenameMap->jsonStatFileUpdateTS(getLocalStorePath(), id, json, true)) {
            return ProcessGetRequestResult::sendInternalError;
         }
      }
      else if (httpRequest.getUriArgs().size() == 4 && 
               httpRequest.getUriArgs()[1] == HTTP_URIPFX_FILES &&
               httpRequest.getUriArgs()[3] == HTTP_URISFX_ZIP) 
      {
         std::string fileName;
         const auto &id = httpRequest.getUriArgs()[2];
         if (!_filenameMap->locked_search(id, fileName)) {
            return ProcessGetRequestResult::sendNotFound;
         }
         httpRequest.setFileName(fileName);
         return ProcessGetRequestResult::sendZipFile;
      }

      return ProcessGetRequestResult::sendErrorInvalidRequest;
   }

};


/* -------------------------------------------------------------------------- */

// Handles the HTTP server request
// This method executes in a specific thread context for
// each accepted HTTP request
void HttpServerSession::operator()(Handle task_handle)
{
   (void)task_handle;

   const int sd = getTcpSocketHandle()->getSocketFd();

   // Generates an identifier for recognizing the transaction
   auto transactionId = [sd]() {
      return "[" + std::to_string(sd) + "] "
         + "[" + SysUtils::getLocalTime() + "] ";
   };

   if (_verboseModeOn)
      log() << transactionId() << "---- http_server_task +\n\n";

   // Wait for a request from remote peer
   HttpRequest::Handle httpRequest(new (std::nothrow) HttpRequest);

   // Wait for a request from remote peer
   while (getTcpSocketHandle()) {
      // Create an http socket around a connected tcp socket
      HttpSocket httpSocket(getTcpSocketHandle());

      if (!httpRequest) {
         if (_verboseModeOn)
            log() << transactionId() << "FATAL ERROR: no memory?\n\n";
         break;
      }

      httpSocket >> httpRequest;

      // If an error occoured terminate the task
      if (!httpSocket) {
         break;
      }

      // Log the request
      if (_verboseModeOn)
         httpRequest->dump(log(), transactionId());

      auto fileName = httpRequest->getFileName();
      std::string jsonResponse;
      ProcessGetRequestResult getRequestAction = ProcessGetRequestResult::none;

      if (httpRequest->getMethod() == HttpRequest::Method::POST
         && httpRequest->getUri() == HTTP_SERVER_POST_STORE
         && !httpRequest->isExpectedContinueResponse()
         && !fileName.empty())
      {
         if (_verboseModeOn)
            log() << transactionId() << "Writing '" << fileName << "'" << std::endl;

         if (!writePostedFile(fileName, httpRequest->getBody(), jsonResponse)) {
            if (_verboseModeOn) {
               log() << transactionId() << "Error writing '"
                  << fileName << "'" << std::endl;
            }
         }
      }
      else if (httpRequest->getMethod() == HttpRequest::Method::GET) {
         getRequestAction = handleGetReq(*httpRequest, jsonResponse);
         fileName = 
            getRequestAction== ProcessGetRequestResult::sendZipFile ?
               httpRequest->getFileName() : "";
      }

      // Format a response to previous HTTP request
      HttpResponse response(
         *httpRequest,
         fileName, 
         getLocalStorePath(),
         jsonResponse,
         jsonResponse.empty() ? "" : ".json");

      // Send the response to remote peer
      httpSocket << response;

      if (getRequestAction == ProcessGetRequestResult::sendZipFile) {
         
         if (0 > httpSocket.sendFile(response.getLocalUriPath())) {
            if (_verboseModeOn)
               log() << transactionId() << "Error sending '"
               << response.getLocalUriPath() << "'\n\n";
            break;
         }
      }
     

      if (_verboseModeOn)
         response.dump(log(), transactionId());

      if (!httpRequest->isExpectedContinueResponse()) {
         httpRequest.reset(new (std::nothrow) HttpRequest);
      }
      else {
         httpRequest->clearExpectedContinueFlag();
      }
   }

   getTcpSocketHandle()->shutdown();

   if (_verboseModeOn) {
      log() << transactionId() << "---- http_server_task -\n\n";
      log().flush();
   }
}


/* -------------------------------------------------------------------------- */
// HttpServer


/* -------------------------------------------------------------------------- */

auto HttpServer::getInstance() -> HttpServer&
{
   if (!_instance) {
      _instance = new (std::nothrow) HttpServer();
      if (!_instance) {
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

   if (!_tcpServer) {
      return false;
   }

   return _tcpServer->bind(port);
}


/* -------------------------------------------------------------------------- */

bool HttpServer::listen(int maxConnections)
{
   if (!_tcpServer) {
      return false;
   }

   return _tcpServer->listen(maxConnections);
}


/* -------------------------------------------------------------------------- */

bool HttpServer::run()
{
   // Create a thread for each TCP accepted connection and
   // delegate it to handle HTTP request / response
   while (true) {
      const TcpSocket::Handle handle = accept();

      // Accept failed for some reason, retry later
      if (!handle) {
         std::this_thread::sleep_for(std::chrono::seconds(1));
         continue;
      }

      assert(_loggerOStreamPtr);

      HttpServerSession::Handle sessionHandle = HttpServerSession::create(
         _verboseModeOn,
         *_loggerOStreamPtr,
         handle,
         _filenameMap,
         _fileStore);

      // Coping the http_server_task handle (shared_ptr) the reference
      // count is automatically increased by one
      std::thread workerThread(*sessionHandle, sessionHandle);

      workerThread.detach();
   }

   // Ok, following instruction won't be ever executed
   return true;
}


/* -------------------------------------------------------------------------- */

HttpServer* HttpServer::_instance = nullptr;

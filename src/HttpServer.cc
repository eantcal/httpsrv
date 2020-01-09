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
      FileRepository::Handle FileRepository
      )
   {
      return Handle(new (std::nothrow) HttpServerSession(
         verboseModeOn,
         loggerOStream,
         socketHandle,
         filenameMap,
         FileRepository));
   }

   HttpServerSession() = delete;
   void operator()(Handle taskHandle);

private:
   bool _verboseModeOn = true;
   std::ostream& _logger;
   TcpSocket::Handle _tcpSocketHandle;
   FilenameMap::Handle _filenameMap;
   FileRepository::Handle _FileRepository;

   std::ostream& log() {
      return _logger;
   }

   TcpSocket::Handle& getTcpSocketHandle() {
      return _tcpSocketHandle;
   }

   const std::string& getLocalStorePath() const {
      return _FileRepository->getPath();
   }

   HttpServerSession(
      bool verboseModeOn, 
      std::ostream& loggerOStream,
      TcpSocket::Handle socketHandle, 
      FilenameMap::Handle filenameMap,
      FileRepository::Handle FileRepository)
      : _verboseModeOn(verboseModeOn)
      , _logger(loggerOStream)
      , _tcpSocketHandle(socketHandle)
      , _filenameMap(filenameMap)
      , _FileRepository(FileRepository)
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
      sendMruFiles,
      sendNotFound,
      sendZipFile
   };

   std::string logBegin() {
      // Generates an identifier for recognizing HTTP session
      std::string sessionId;

      if (_verboseModeOn) {
         const int sd = getTcpSocketHandle()->getSocketFd();
         sessionId =
            "[" + std::to_string(sd) + "] " +
            "[" + SysUtils::getLocalTime() + "] ";

         log() << sessionId << "---- HTTP SERVER SESSION STARTS" << std::endl;
         log().flush();
      }
      return sessionId;
   }

   void logEnd(const std::string& sessionId) {
      if (_verboseModeOn) {
         log() << sessionId << "---- HTTP SERVER SESSION ENDS" << std::endl << std::endl;
         log().flush();
      }
   }

   ProcessGetRequestResult processGetRequest(
      HttpRequest& httpRequest, 
      std::string& json,
      std::string& fileToSend)
   {
      const auto& uri = httpRequest.getUri();

      if (uri == HTTP_SERVER_GET_FILES) {
         if (_filenameMap->locked_updateMakeJson(getLocalStorePath(), json)) {
            return ProcessGetRequestResult::sendJsonFileList;
         }
      }
      
      if (uri == HTTP_SERVER_GET_MRUFILES) {
         if (!_FileRepository->createJsonMruFilesList(json)) {
            return ProcessGetRequestResult::sendInternalError;
         }
         return ProcessGetRequestResult::sendMruFiles;
      }

      if (uri == HTTP_SERVER_GET_MRUFILES_ZIP) {
         fs::path tempDir;
         if (!FileUtils::createTemporaryDir(tempDir)) {
            return ProcessGetRequestResult::sendInternalError;
         }

         std::list<std::string> fileList;
         if (!_FileRepository->createMruFilesList(fileList)) {
            return ProcessGetRequestResult::sendInternalError;
         }
   
         tempDir /= MRU_FILES_ZIP_NAME;
         ZipArchive zipArchive(tempDir.string());
         if (!zipArchive.create()) {
            return ProcessGetRequestResult::sendInternalError;
         }

         for (const auto& fileName: fileList) {
            fs::path src(getLocalStorePath());
            src /= fileName;
         
            if (!zipArchive.add(src.string(), fileName)) {
               return ProcessGetRequestResult::sendInternalError;
            }
         }

         zipArchive.close();         
         fileToSend = tempDir.string();

         return ProcessGetRequestResult::sendZipFile;
      }

      const auto& uriArgs = httpRequest.getUriArgs();

      if (uriArgs.size() == 3 && uriArgs[1] == HTTP_URIPFX_FILES) {
         const auto &id = httpRequest.getUriArgs()[2];
         if (!_filenameMap->jsonStatFileUpdateTS(getLocalStorePath(), id, json, true)) {
            return ProcessGetRequestResult::sendInternalError;
         }
      }
      
      if (uriArgs.size() == 4 && uriArgs[1] == HTTP_URIPFX_FILES && uriArgs[3] == HTTP_URISFX_ZIP) {
         std::string fileName;
         const auto &id = uriArgs[2];

         if (!_filenameMap->locked_search(id, fileName)) {
            return ProcessGetRequestResult::sendNotFound;
         }
         
         fs::path tempDir;
         if (!FileUtils::createTemporaryDir(tempDir)) {
            return ProcessGetRequestResult::sendInternalError;
         }
         
         tempDir /= fileName + ".zip";
         fs::path src(getLocalStorePath());
         src /= fileName;

         const auto updated = 
            FileUtils::touch(src.string(), false /*== do not create if it does not exist*/);
         
         ZipArchive zipArchive(tempDir.string());
         if (!updated || !zipArchive.create() || !zipArchive.add(src.string(), fileName)) {
            return ProcessGetRequestResult::sendInternalError;
         }
         
         zipArchive.close();
         
         fileToSend = tempDir.string();

         return ProcessGetRequestResult::sendZipFile;
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

   // Wait for a request from remote peer
   while (getTcpSocketHandle()) {
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

      if (httpRequest->isExpectedContinueResponse() || httpRequest->isValidPostRequest()) {
         auto fileName = httpRequest->getFileName();

         if (_verboseModeOn) {
            log() << sessionId << "Writing '" << fileName << "'" << std::endl;
            log().flush();
         }

         if (!writePostedFile(fileName, httpRequest->getBody(), jsonResponse)) {
            if (_verboseModeOn) {
               log() << sessionId << "Error writing '" << fileName << "'" << std::endl;
               log().flush();
            }
         }
      }
      else if (httpRequest->isValidGetRequest()) {
         getRequestAction = processGetRequest(*httpRequest, jsonResponse, fileToSend);
      }
      else {
         response = std::move(std::make_unique<HttpResponse>(400)); // Bad Request
      }
         
      if (! response) {
         // Format a response to previous HTTP request, unless a bad
         // request was detected
         response= std::move(std::make_unique<HttpResponse>(
            *httpRequest,
            jsonResponse,
            jsonResponse.empty() ? "" : ".json",
            fileToSend));
      }

      assert(response);

      // Send the response header and any not empty json content to remote peer
      httpSocket << *response;

      // Any binary content is sent after response header
      if (getRequestAction == ProcessGetRequestResult::sendZipFile) {
         if (0 > httpSocket.sendFile(fileToSend)) {
            if (_verboseModeOn) {
               log() << sessionId << "Error sending '" << fileToSend 
                     << "'" << std::endl << std::endl;
               log().flush();
            }
            break;
         }
      }

      if (_verboseModeOn)
         response->dump(log(), sessionId);

      if (response->isErrorResponse())
         break;

      if (!httpRequest->isExpectedContinueResponse()) {
         httpRequest.reset(new (std::nothrow) HttpRequest);
         assert(httpRequest);
      }
      else {
         httpRequest->clearExpectedContinueFlag();
      }
   }

   getTcpSocketHandle()->shutdown();

   logEnd(sessionId);
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

HttpServer* HttpServer::_instance = nullptr;

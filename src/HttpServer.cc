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
#include "Tools.h"

#include <thread>
#include <cassert>


/* -------------------------------------------------------------------------- */
// HttpServerTask

/* -------------------------------------------------------------------------- */

class HttpServerTask {
public:
   using Handle = std::shared_ptr<HttpServerTask>;

   inline static Handle create(
      bool verboseModeOn,
      std::ostream& loggerOStream,
      TcpSocket::Handle socketHandle,
      const std::string& webRootPath)
   {
      return Handle(new HttpServerTask(
         verboseModeOn,
         loggerOStream,
         socketHandle,
         webRootPath));
   }

   HttpServerTask() = delete;
   void operator()(Handle task_handle);

private:
   bool _verboseModeOn = true;
   std::ostream& _logger;
   TcpSocket::Handle _tcpSocketHandle;
   std::string _webRootPath;

   std::ostream& log() {
      return _logger;
   }

   bool verboseModeOn() const {
      return _verboseModeOn;
   }

   TcpSocket::Handle& getTcpSocketHandle() {
      return _tcpSocketHandle;
   }

   const std::string& getWebRootPath() const {
      return _webRootPath;
   }

   HttpServerTask(bool verboseModeOn, std::ostream& loggerOStream,
      TcpSocket::Handle socketHandle, const std::string& webRootPath)
      : _verboseModeOn(verboseModeOn)
      , _logger(loggerOStream)
      , _tcpSocketHandle(socketHandle)
      , _webRootPath(webRootPath)
   {
   }

};


/* -------------------------------------------------------------------------- */

// Handles the HTTP server request
// This method executes in a specific thread context for
// each accepted HTTP request
void HttpServerTask::operator()(Handle task_handle)
{
   (void)task_handle;

   const int sd = getTcpSocketHandle()->getSocketFd();

   // Generates an identifier for recognizing the transaction
   auto transactionId = [sd]() {
      return "[" + std::to_string(sd) + "] "
         + "[" + Tools::getLocalTime() + "] ";
   };

   if (verboseModeOn())
      log() << transactionId() << "---- http_server_task +\n\n";

   // Wait for a request from remote peer
   HttpRequest::Handle httpRequest(new (std::nothrow) HttpRequest);

   // Wait for a request from remote peer
   while (getTcpSocketHandle()) {
      // Create an http socket around a connected tcp socket
      HttpSocket httpSocket(getTcpSocketHandle());

      if (!httpRequest) {
         if (verboseModeOn())
            log() << transactionId() << "FATAL ERROR: no memory?\n\n";
         break;
      }

      httpSocket >> httpRequest;

      // If an error occoured terminate the task
      if (!httpSocket) {
         break;
      }

      // Log the request
      if (verboseModeOn())
         httpRequest->dump(log(), transactionId());

      const auto& fileName = httpRequest->getFileName();
      std::string responseBody;

      if (httpRequest->getMethod() == HttpRequest::Method::POST
         && httpRequest->getUri() == HTTP_SERVER_POST_STORE
         && !httpRequest->isExpectedContinueResponse()
         && !fileName.empty())
      {
         std::string filePath = getWebRootPath() + "/" + fileName;

         if (verboseModeOn())
            log() << transactionId() << "Saving body content into '"
            << filePath << "'\n\n";

         std::ofstream os(filePath, std::ofstream::binary);

         const auto& requestBody = httpRequest->getBody();
         os.write(requestBody.data(), requestBody.size());

         if (!os.fail()) {
            os.close();
            if (!Tools::jsonStat(filePath, fileName, responseBody)) {
               responseBody.clear(); // will create a 500 error response
            }
         }
         else if (verboseModeOn()) {
            log() << transactionId() << "Error saving file '"
               << filePath << "'\n\n";
         }
      }

      // Build a response to previous HTTP request
      HttpResponse response(
         *httpRequest,
         getWebRootPath(),
         responseBody,
         responseBody.empty() ? "" : ".json");

      // Send the response to remote peer
      httpSocket << response;

      // If HTTP command line method isn't HEAD then send requested URI
      if (httpRequest->getMethod() == HttpRequest::Method::GET) {
         if (0 > httpSocket.sendFile(response.getLocalUriPath())) {
            if (verboseModeOn())
               log() << transactionId() << "Error sending '"
               << response.getLocalUriPath() << "'\n\n";
            break;
         }
      }

      if (verboseModeOn())
         response.dump(log(), transactionId());

      if (!httpRequest->isExpectedContinueResponse()) {
         httpRequest.reset(new (std::nothrow) HttpRequest);
      }
      else {
         httpRequest->clearExpectedContinueFlag();
      }
   }

   getTcpSocketHandle()->shutdown();

   if (verboseModeOn()) {
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

      HttpServerTask::Handle taskHandle = HttpServerTask::create(
         _verboseModeOn,
         *_loggerOStreamPtr,
         handle,
         getWebRootPath());

      // Coping the http_server_task handle (shared_ptr) the reference
      // count is automatically increased by one
      std::thread workerThread(*taskHandle, taskHandle);

      workerThread.detach();
   }

   // Ok, following instruction won't be ever executed
   return true;
}


/* -------------------------------------------------------------------------- */

HttpServer* HttpServer::_instance = nullptr;

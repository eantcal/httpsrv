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
#include "HttpSession.h"

#include <thread>

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

      assert(_loggerOStreamPtr);

      // Accept failed for some reason, retry later
      if (!handle)
      {
         if (_verboseModeOn) 
         {
            *_loggerOStreamPtr 
               << "HttpServer::run() accept is failing" << std::endl;
         }
         std::this_thread::sleep_for(std::chrono::seconds(1));
         continue;
      }

      // Create a new http session context
      HttpSession::Handle sessionHandle = HttpSession::create(
          _verboseModeOn,
          *_loggerOStreamPtr,
          handle,
          _FileRepository);

      // the function operator() will because of *sessionHandle
      // reference, while the sessionHandle itself will be
      // copied to the thread stack as parameter, this will force
      // to increment the handle reference count until the worker thread
      // is running preserving the context integrity
      std::thread workerThread(*sessionHandle, sessionHandle);

      workerThread.detach();
   }

   // Ok, following instruction won't be ever executed
   return true;
}

/* -------------------------------------------------------------------------- */

HttpServer *HttpServer::_instance = nullptr;

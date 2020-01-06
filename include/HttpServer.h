//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__


/* -------------------------------------------------------------------------- */

#include "HttpSocket.h"
#include "TcpListener.h"
#include "FilenameMap.h"

#include "config.h"

#include <iostream>
#include <string>


/* -------------------------------------------------------------------------- */

/**
 * The top-level class of the HTTP server.
 */
class HttpServer {
public:
   using TranspPort = TcpListener::TranspPort;

public:
   HttpServer(const HttpServer&) = delete;
   HttpServer& operator=(const HttpServer&) = delete;


   /**
    * Sets a loggerOStream enabling the verbose mode.
    *
    * @param pointer to ostream used for logging
    */
   void setupLogger(std::ostream* loggerOStream = nullptr) {
      if (loggerOStream) {
         _loggerOStreamPtr = loggerOStream;
         _verboseModeOn = true;
      }
      else {
         _verboseModeOn = false;
      }
   }

   /**
    * Sets a id/filename cache instance
    */
   void setIdFileNameCache(FilenameMap::Handle cacheInstance) noexcept {
      _idFileNameCache = cacheInstance;
   }

   /**
    * Sets mrufiles number
    */
   void setMruFilesNumber(int mrufilesN) noexcept {
      _mrufilesN = mrufilesN;
   }

   /**
    * Gets HttpServer object instance reference.
    * This class is a singleton. First time this function is called,
    * the HttpServer object is initialized.
    *
    * @return the HttpServer reference
    */
   static auto getInstance()->HttpServer&;

   /**
    * Gets current server working directory
    */
   const std::string& getLocalStorePath() const noexcept {
      return _localStorePath;
   }

   /**
    * Sets the server working directory
    */
   void setLocalStorePath(const std::string& path) {
      _localStorePath = path;
   }

   /**
    * Gets the port where server is listening
    *
    * @return the port number
    */
   TranspPort getLocalPort() const {
      return _serverPort;
   }

   /**
    * Binds the HTTP server to a local TCP port
    *
    * @param port listening port
    * @return true if operation is successfully completed, false otherwise
    */
   bool bind(TranspPort port);

   /**
    * Sets the server in listening mode
    *
    * @param maxConnections back log list length
    * @return true if operation is successfully completed, false otherwise
    */
   bool listen(int maxConnections);

   /**
    * Runs the server. This function is blocking for the caller.
    *
    * @return false if operation failed, otherwise the function
    * doesn't return ever
    */
   bool run();


protected:
   /**
    * Accepts a new connection from a remote client.
    * This function blocks until connection is established or
    * an error occurs.
    * @return a handle to tcp socket
    */
   TcpSocket::Handle accept() {
      return _tcpServer ? _tcpServer->accept() : nullptr;
   }

private:
   std::ostream* _loggerOStreamPtr = &std::clog;
   static HttpServer* _instance;
   TranspPort _serverPort = HTTP_SERVER_PORT;
   TcpListener::Handle _tcpServer;
   std::string _localStorePath = "/tmp";
   bool _verboseModeOn = true;
   FilenameMap::Handle _idFileNameCache;
   int _mrufilesN = MRUFILES_DEF_N;

   HttpServer() = default;

};


/* -------------------------------------------------------------------------- */

#endif // __HTTP_SERVER_H__

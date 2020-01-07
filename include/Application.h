//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __APPLICATION_H__
#define __APPLICATION_H__


/* -------------------------------------------------------------------------- */

#include "HttpServer.h"
#include "FileRepository.h"
#include "FilenameMap.h"
#include "FileUtils.h"
#include "SysUtils.h"
#include "StrUtils.h"
#include "TcpSocket.h"

#include <memory>
#include <string>
#include <iostream>
#include <sstream>

#include "config.h"


/* -------------------------------------------------------------------------- */

//! Helper class to manage the application configuration and server startup
class Application {
public:
   enum class ErrCode {
      success,
      commandLineError,
      showVersionUsage,
      fileRepositoryInitError,
      idFileNameCacheInitError,
      commLibError,
      httpSrvBindError,
      httpSrvListenError,
      httpSrvStartError,
   };

   //! Initializes the application based on input paramters
   Application(int argc, char* argv[], std::ostream& logger);

   //! Applies a given configuration and runs HTTP Server
   ErrCode run();

   //! Gets any specific error message based on run() return-value
   const std::string& getError() const noexcept {
      return _errMessage;
   }

private:
   Application(const Application&) = delete;
   Application& operator=(const Application&) = delete;

   bool showUsage(std::stringstream& os) const;

   std::ostream& _logger;

   FilenameMap::Handle _filenameMap;

   std::string _progName;
   std::string _commandLine;
   std::string _localStorePath = HTTP_SERVER_LOCAL_STORE_PATH;

   TcpSocket::TranspPort _httpServerPort = HTTP_SERVER_PORT;

   bool _showHelp = false;
   bool _showVer = false;
   bool _error = false;
   bool _verboseModeOn = false;
   std::string _errMessage;

   static const int _min_ver = HTTP_SERVER_MIN_V;
   static const int _maj_ver = HTTP_SERVER_MAJ_V;

   int _mrufilesN = MRUFILES_DEF_N;

   FileRepository::Handle _FileRepository;
};


/* -------------------------------------------------------------------------- */

#endif //! __APPLICATION_H__

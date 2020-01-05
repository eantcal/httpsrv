//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "HttpServer.h"
#include "IdFileNameCache.h"

#include "FileUtils.h"
#include "SysUtils.h"
#include "StrUtils.h"

#include "ProgOpt.h"

#include <iostream>
#include <string>
#include <map>
#include <unordered_map>


/* -------------------------------------------------------------------------- */

class Application {
public:
   static Application& getInstance() {
      static Application applicationInstance;
      return applicationInstance;
   }

   enum class ErrCode {
      success,
      fileRepositoryInitError,
      idFileNameCacheInitError,
      socketInitiError,
      configurationError,
      httpSrvBindError,
      httpSrvListenError,
   };

   ErrCode init(int argc, char* argv[], std::ostream& logger = std::clog) {
      // Initialize any O/S specific libraries
      if (!SysUtils::initCommunicationLib()) {
         return ErrCode::socketInitiError;
      }

      _configuration = ProgOpt::make(argc, argv);
      if (!_configuration || !_configuration->isValid()) {
         return ErrCode::configurationError;
      }

      // Creates or validates (if already existant) a repository for text file
      _idFileNameCache = IdFileNameCache::make();
      if (!_idFileNameCache) {
         return ErrCode::idFileNameCacheInitError;
      }
      
      auto repositoryPath = FileUtils::initRepository(
         _configuration->getWebRootPath());

      if(repositoryPath.empty() || 
         !FileUtils::scanRepository(
            repositoryPath,
            _timeOrderedFileListCache,
            *_idFileNameCache))
      {
         return ErrCode::fileRepositoryInitError;
      }

      for (auto it = _timeOrderedFileListCache.rbegin();
           it != _timeOrderedFileListCache.rend(); 
           ++it)
      {
         std::cerr << it->second << std::endl;
      }
      //...tmp

      auto& httpSrv = HttpServer::getInstance();

      // file repository dir name is canonical full path name
      httpSrv.setupWebRootPath(repositoryPath);

      if (!httpSrv.bind(_configuration->getHttpServerPort())) {
         return ErrCode::httpSrvBindError;
      }

      if (!httpSrv.listen(HTTP_SERVER_BACKLOG)) {
         return ErrCode::httpSrvListenError;
      }

      httpSrv.setupLogger(_configuration->verboseModeOn() ? &logger : nullptr);

      return ErrCode::success;
   }

   bool runServer() {
      auto & httpServerInstance = HttpServer::getInstance();

      assert(_idFileNameCache);

      // Configure the server
      httpServerInstance.setIdFileNameCache(_idFileNameCache);

      return httpServerInstance.run();
   }

   ProgOpt::Handle getConfiguration() const noexcept {
      return _configuration;
   }

private:
   Application() = default;
   Application(const Application&) = delete;
   Application& operator=(const Application&) = delete;

   // Parse the command line
   ProgOpt::Handle _configuration;
   FileUtils::TimeOrderedFileList _timeOrderedFileListCache;
   IdFileNameCache::Handle _idFileNameCache;
};


/* -------------------------------------------------------------------------- */

/**
 * Program entry point
 */
int main(int argc, char* argv[])
{
   Application& application = Application::getInstance();
   auto errCode = application.init(argc, argv);
   auto cfg = application.getConfiguration();

   if (!cfg) {
      std::cerr << "Fatal error, out of memory?" << std::endl;
      return 1;
   }

   if (cfg->showUsage(std::cerr)) {
      return 0;
   }

   switch (errCode) {
   case Application::ErrCode::success:
   default:
      break;

   case Application::ErrCode::fileRepositoryInitError: {
      std::cerr << "Cannot initialize the file repository" << std::endl;
      return 1;
   }

   case Application::ErrCode::idFileNameCacheInitError: {
      std::cerr << "Cannot initialize the filename cache" << std::endl;
      return 1;
   }                           

   case Application::ErrCode::configurationError: {
      std::cerr << cfg->error() << std::endl;
      return 1;
   }

   case Application::ErrCode::httpSrvBindError: {
      std::cerr << "Error binding server port "
         << cfg->getHttpServerPort() << std::endl;

      return 1;
   }

   case Application::ErrCode::httpSrvListenError: {
      std::cerr << "Error trying to listen to server port "
         << cfg->getHttpServerPort() << std::endl;

      return 1;
   }
   }

   if (cfg->verboseModeOn()) {
      std::clog << SysUtils::getLocalTime() << std::endl
         << "Command line :'" << cfg->getCommandLine() << "'"
         << std::endl
         << HTTP_SERVER_NAME << " is listening on TCP port "
         << cfg->getHttpServerPort() << std::endl
         << "Working directory is '" << cfg->getWebRootPath() << "'\n";
   }

   if (!application.runServer()) {
      std::cerr << "Error starting the server\n";
      return 1;
   }

   return 0;
}

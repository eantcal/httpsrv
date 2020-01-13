//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "Application.h"

/* -------------------------------------------------------------------------- */

bool Application::showUsage(std::stringstream &os) const
{
   if (_showVer)
      os << HTTPSRV_NAME << " " << _maj_ver << "." << _min_ver
         << "\n";

   if (!_showHelp)
      return _showVer;

   os << "Usage:\n";
   os << "\t" << _progName << "\n";
   os << "\t\t-p | --port <port>\n";
   os << "\t\t\tBind server to a TCP port number (default is "
      << HTTPSRV_PORT << ") \n";
   os << "\t\t-n | --mrufiles <N>\n";
   os << "\t\t\tMRU Files N (default is "
      << MRUFILES_DEF_N << ") \n";
   os << "\t\t-w | --storedir <repository-path>\n";
   os << "\t\t\tSet a repository directory (default is "
      << HTTPSRV_LOCAL_REPOSITORY_PATH << ") \n";
   os << "\t\t-vv | --verbose\n";
   os << "\t\t\tEnable logging on stderr\n";
   os << "\t\t-v | --version\n";
   os << "\t\t\tShow software version\n";
   os << "\t\t-h | --help\n";
   os << "\t\t\tShow this help \n";

   return true;
}

/* -------------------------------------------------------------------------- */

Application::Application(
    int argc,
    char *argv[],
    std::ostream &logger)
    : 
    _logger(logger)
{
   assert(argc > 0);

   _progName = argv[0];
   _commandLine = _progName;

   if (argc <= 1)
      return;

   enum class State
   {
      OPTION,
      PORT,
      WEBROOT,
      MRUFILES_N
   }
   state = State::OPTION;

   for (int idx = 1; idx < argc; ++idx)
   {
      std::string sarg = argv[idx];

      _commandLine += " ";
      _commandLine += sarg;

      switch (state)
      {
      case State::OPTION:
         if (sarg == "--port" || sarg == "-p")
         {
            state = State::PORT;
         }
         else if (sarg == "--mrufiles" || sarg == "-n")
         {
            state = State::MRUFILES_N;
         }
         else if (sarg == "--storedir" || sarg == "-w")
         {
            state = State::WEBROOT;
         }
         else if (sarg == "--help" || sarg == "-h")
         {
            _showHelp = true;
            state = State::OPTION;
         }
         else if (sarg == "--version" || sarg == "-v")
         {
            _showVer = true;
            state = State::OPTION;
         }
         else if (sarg == "--verbose" || sarg == "-vv")
         {
            _verboseModeOn = true;
            state = State::OPTION;
         }
         else
         {
            _errMessage = "Unknown option '" + sarg 
                        + "', try with --help or -h";
            _error = true;
            return;
         }
         break;

      case State::WEBROOT:
         _localRepositoryPath = sarg;
         state = State::OPTION;
         break;

      case State::PORT:
         try
         {
            _httpServerPort = std::stoi(sarg);
            if (_httpServerPort < 1)
               throw 0;
         }
         catch (...)
         {
            _errMessage = "Invalid port number";
            _error = true;
            return;
         }
         state = State::OPTION;
         break;

      case State::MRUFILES_N:
         try
         {
            _mrufilesN = std::stoi(sarg);
            if (_mrufilesN < 1 || _mrufilesN > MRUFILES_MAX_N)
               throw 0;
         }
         catch (...)
         {
            _errMessage = "Invalid mrufiles number";
            _error = true;
            return;
         }
         state = State::OPTION;
         break;
      }
   }
}

/* -------------------------------------------------------------------------- */

Application::ErrCode Application::run()
{
   if (_error)
      return ErrCode::commandLineError;

   std::stringstream ss;
   if (showUsage(ss))
   {
      _errMessage = ss.str();
      return ErrCode::commandLineError;
   }

   // Initialize any O/S specific libraries
   if (!SysUtils::initCommunicationLib())
   {
      _errMessage = "Cannot initialize communication library";
      return ErrCode::commLibError;
   }

   _FileRepository = FileRepository::make(_localRepositoryPath, _mrufilesN);

   // Scan the repository for mapping hash(filename)->filename 
   if (!_FileRepository ||
       !_FileRepository->getFilenameMap().scan(_FileRepository->getPath()))
   {
      _errMessage = "Cannot initialize the local repository";
      return ErrCode::fileRepositoryInitError;
   }

   // Creates the HttpServer instance
   auto &httpSrv = HttpServer::getInstance();

   httpSrv.setFileRepository(_FileRepository);

   // Bind the server to any-interface:_httpServerPort
   if (!httpSrv.bind(_httpServerPort))
   {
      ss << "Error binding server port " << _httpServerPort;
      _errMessage = ss.str();
      return ErrCode::httpSrvBindError;
   }

   // Make it listening configuring the backlog list size
   if (!httpSrv.listen(HTTPSRV_BACKLOG))
   {
      ss << "Error trying to listen to server port "
         << _httpServerPort;

      _errMessage = ss.str();

      return ErrCode::httpSrvListenError;
   }

   // If required set a logger
   httpSrv.setupLogger(_verboseModeOn ? &_logger : nullptr);

   if (_verboseModeOn)
   {
      std::cout << SysUtils::getUtcTime() << std::endl
                << "Command line :'" << _commandLine << "'"
                << std::endl
                << HTTPSRV_NAME << " is listening on TCP port "
                << _httpServerPort << std::endl
                << "Working directory is '" << _localRepositoryPath << "'"
                << std::endl;
   }

   // Finally run the server (blocking the caller)
   if (!HttpServer::getInstance().run())
   {
      _errMessage = "Error starting the server";
      return ErrCode::httpSrvStartError;
   }

   // Following statement won't be executed
   return ErrCode::success;
}

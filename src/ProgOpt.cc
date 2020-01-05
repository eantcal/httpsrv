//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "ProgOpt.h"


/* -------------------------------------------------------------------------- */

bool ProgOpt::showUsage(std::ostream& os) const {
   if (_show_ver)
      os << HTTP_SERVER_NAME << " " << _maj_ver << "." << _min_ver
      << "\n";

   if (!_show_help)
      return _show_ver;

   os << "Usage:\n";
   os << "\t" << getProgName() << "\n";
   os << "\t\t-p | --port <port>\n";
   os << "\t\t\tBind server to a TCP port number (default is "
      << HTTP_SERVER_PORT << ") \n";
   os << "\t\t-w | --webroot <working_dir_path>\n";
   os << "\t\t\tSet a local working directory (default is "
      << HTTP_SERVER_WROOT << ") \n";
   os << "\t\t-vv | --verbose\n";
   os << "\t\t\tEnable logging on stderr\n";
   os << "\t\t-v | --version\n";
   os << "\t\t\tShow software version\n";
   os << "\t\t-h | --help\n";
   os << "\t\t\tShow this help \n";

   return true;
}


/* -------------------------------------------------------------------------- */

ProgOpt::ProgOpt(int argc, char* argv[]) {
   if (!argc)
      return;

   _progName = argv[0];
   _commandLine = _progName;

   if (argc <= 1)
      return;

   enum class State { OPTION, PORT, WEBROOT } state = State::OPTION;

   for (int idx = 1; idx < argc; ++idx) {
      std::string sarg = argv[idx];

      _commandLine += " ";
      _commandLine += sarg;

      switch (state) {
      case State::OPTION:
         if (sarg == "--port" || sarg == "-p") {
            state = State::PORT;
         }
         else if (sarg == "--webroot" || sarg == "-w") {
            state = State::WEBROOT;
         }
         else if (sarg == "--help" || sarg == "-h") {
            _show_help = true;
            state = State::OPTION;
         }
         else if (sarg == "--version" || sarg == "-v") {
            _show_ver = true;
            state = State::OPTION;
         }
         else if (sarg == "--verbose" || sarg == "-vv") {
            _verboseModeOn = true;
            state = State::OPTION;
         }
         else {
            _err_msg = "Unknown option '" + sarg
               + "', try with --help or -h";
            _error = true;
            return;
         }
         break;

      case State::WEBROOT:
         _webRootPath = sarg;
         state = State::OPTION;
         break;

      case State::PORT:
         try {
            _http_server_port = std::stoi(sarg);
            if (_http_server_port < 1)
               throw 0;
         }
         catch (...) {
            _err_msg = "Invalid port number";
            _error = true;
            return;
         }
         state = State::OPTION;
         break;
      }
   }
}

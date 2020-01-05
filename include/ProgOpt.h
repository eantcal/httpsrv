//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __PROG_OPT_H__
#define __PROG_OPT_H__


/* -------------------------------------------------------------------------- */

#include <memory>
#include <string>
#include <iostream>

#include "TcpSocket.h"

#include "config.h"


/* -------------------------------------------------------------------------- */

//! Helper class used by Application to read program parameters (configuration)
class ProgOpt {
public:
   using Handle = std::shared_ptr<ProgOpt>;

   ProgOpt() = delete;

   static Handle make(int argc, char* argv[]) {
      return Handle(new (std::nothrow) ProgOpt(argc, argv));
   }

   const std::string& getProgName() const noexcept {
      return _progName;
   }

   const std::string& getCommandLine() const noexcept {
      return _commandLine;
   }

   const std::string& getWebRootPath() const noexcept {
      return _webRootPath;
   }

   TcpSocket::TranspPort getHttpServerPort() const noexcept {
      return _http_server_port;
   }

   bool isValid() const noexcept {
      return !_error;
   }

   bool verboseModeOn() const noexcept {
      return _verboseModeOn;
   }

   const std::string& error() const noexcept {
      return _err_msg;
   }

   /*
    * Writes usage text in output stream
    * @param os is the output stream
    * @return true if the usage help has been written (depending on cfg), false
    *         otherwise
    */
   bool showUsage(std::ostream& os) const;

   /* -------------------------------------------------------------------------- */

   /**
    * Parse the command line getting initialization configuration
    */
   ProgOpt(int argc, char* argv[]);

private:
   std::string _progName;
   std::string _commandLine;
   std::string _webRootPath = HTTP_SERVER_WROOT;

   TcpSocket::TranspPort _http_server_port = HTTP_SERVER_PORT;

   bool _show_help = false;
   bool _show_ver = false;
   bool _error = false;
   bool _verboseModeOn = false;
   std::string _err_msg;

   static const int _min_ver = HTTP_SERVER_MIN_V;
   static const int _maj_ver = HTTP_SERVER_MAJ_V;
};

#endif

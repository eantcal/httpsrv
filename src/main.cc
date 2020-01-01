//
// This file is part of thttpd
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "HttpServer.h"
#include "Tools.h"

#include <iostream>
#include <string>

/* -------------------------------------------------------------------------- */

class ProgArgsParser {
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

public:
    ProgArgsParser() = delete;


    const std::string& getProgName() const { 
       return _progName; 
    }

    const std::string& getCommandLine() const { 
       return _commandLine; 
    }

    const std::string& getWebRootPath() const { 
       return _webRootPath; 
    }

    TcpSocket::TranspPort getHttpServerPort() const {
        return _http_server_port;
    }


    bool isValid() const { 
       return !_error; 
    }

    bool verboseModeOn() const { 
       return _verboseModeOn; 
    }

    const std::string& error() const { 
       return _err_msg; 
    }

    bool showUsage(std::ostream& os) const {
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

    /**
     * Parse the command line
     */
    ProgArgsParser(int argc, char* argv[]) {
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
                } else if (sarg == "--webroot" || sarg == "-w") {
                    state = State::WEBROOT;
                } else if (sarg == "--help" || sarg == "-h") {
                    _show_help = true;
                    state = State::OPTION;
                } else if (sarg == "--version" || sarg == "-v") {
                    _show_ver = true;
                    state = State::OPTION;
                } else if (sarg == "--verbose" || sarg == "-vv") {
                    _verboseModeOn = true;
                    state = State::OPTION;
                } else {
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
                _http_server_port = std::stoi(sarg);
                state = State::OPTION;
                break;
            }
        }
    }
};


/* -------------------------------------------------------------------------- */

/**
 * Program entry point
 */
int main(int argc, char* argv[])
{
    std::string json;
    Tools::jsonStat("nino.txt", json);
    std::cout << json;
    return 0;

    std::string msg;

    // Initialize any O/S specific libraries
    if (!OsSocketSupport::initSocketLibrary(msg)) {
        
        if (!msg.empty())
            std::cerr << msg << std::endl;

        return 1;
    }


    // Parse the command line
    ProgArgsParser args(argc, argv);

    if (!args.isValid()) {
        std::cerr << args.error() << std::endl;
        return 1;
    }

    if (args.showUsage(std::cout)) {
        return 0;
    }

    HttpServer& httpsrv = HttpServer::getInstance();

    httpsrv.setupWebRootPath(args.getWebRootPath());

    bool res = httpsrv.bind(args.getHttpServerPort());

    if (!res) {
        std::cerr << "Error binding server port " << args.getHttpServerPort()
                  << "\n";
        return 1;
    }

    res = httpsrv.listen(HTTP_SERVER_BACKLOG);
    if (!res) {
        std::cerr << "Error setting listeing mode\n";
        return 1;
    }

    std::cout << Tools::getLocalTime() << std::endl
              << "Command line :'" << args.getCommandLine() << "'"
              << std::endl
              << HTTP_SERVER_NAME << " is listening on TCP port "
              << args.getHttpServerPort() << std::endl
              << "Working directory is '" << args.getWebRootPath() << "'\n";

    httpsrv.setupLogger(args.verboseModeOn() ? &std::clog : nullptr);

    if (!httpsrv.run()) {
        std::cerr << "Error starting the server\n";
        return 1;
    }

    return 0;
}

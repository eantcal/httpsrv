//
// This file is part of httpsrv
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
#include <map>
#include <unordered_map>

#include <boost/filesystem.hpp>

/* -------------------------------------------------------------------------- */

class Configuration {
public:
    using Handle = std::shared_ptr<Configuration>;

    Configuration() = delete;

    static Handle make(int argc, char* argv[]) {
        return Handle( new (std::nothrow) Configuration(argc, argv) );
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
     * Parse the command line getting initialization configuration
     */
    Configuration(int argc, char* argv[]) {
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


/* -------------------------------------------------------------------------- */

class FileRepository {
public:
    using TimeOrderedFileList = std::multimap<std::time_t, boost::filesystem::path>;
    using IdFileNameCache = std::unordered_map<std::string, boost::filesystem::path>;

    using Handle = std::shared_ptr<FileRepository>;

    static Handle make(const std::string& dirName) {
        FileRepository::Handle handle(new (std::nothrow) FileRepository(dirName));
        return handle && !handle->_directoryName.empty() ? handle : nullptr;
    }

    const std::string& getDirName() const noexcept {
        return _directoryName;
    }

    void scan(const std::string& path, TimeOrderedFileList& list)
    {
       boost::filesystem::path dirPath(path);
       boost::filesystem::directory_iterator endIt;

       if ( boost::filesystem::exists(dirPath) && 
            boost::filesystem::is_directory(dirPath)) 
       {
          for( boost::filesystem::directory_iterator it(dirPath); it != endIt; ++it) {
             if (boost::filesystem::is_regular_file(it->status()) ) {
                list.insert({boost::filesystem::last_write_time(it->path()), *it});
             }
          }
       }
    }

private:
    FileRepository(const std::string& dirName)
    {
        // Resolve any homedir prefix
        std::string resPath;
        if (!dirName.empty()) {
            size_t prefixSize = dirName.size() > 1 ? 2 : 1;
            if ((prefixSize >1 && dirName.substr(0,2)=="~/") || dirName=="~") {
                resPath = Tools::getHomeDir();
                resPath += "/";
                resPath += dirName.substr(prefixSize, dirName.size() - prefixSize);
            }
        }

        if (!Tools::touchDir(resPath, _directoryName)) {
            // make sure error is propagate to factory function
            _directoryName.clear();
        }
      
    }

    std::string _directoryName;
};


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
        socketInitiError,
        configurationError,
        httpSrvBindError,
        httpSrvListenError,
    };

    ErrCode init(int argc, char* argv[], std::ostream& logger = std::clog) {
        // Initialize any O/S specific libraries
        if (!OsSpecific::initSocketLibrary()) {
            return ErrCode::socketInitiError;
        }

        _configuration = Configuration::make(argc, argv);
        if (!_configuration || !_configuration->isValid()) {
            return ErrCode::configurationError;
        }

        auto repositoryDir = _configuration->getWebRootPath();

        // Creates or validates (if already existant) a file repository
        _fileRepository = FileRepository::make(repositoryDir);
        if (!_fileRepository) {
            return ErrCode::fileRepositoryInitError;
        }

        //tmp...
        // Get a picture of repository file list
        _fileRepository->scan(
              _fileRepository->getDirName(), 
              _timeOrderedFileListCache);

        for (auto it=_timeOrderedFileListCache.rbegin(); it != _timeOrderedFileListCache.rend(); ++it) {
           std::cerr << it->second << std::endl;
        }
        //...tmp

        auto & httpSrv = HttpServer::getInstance();

        // file repository dir name is canonical full path name
        httpSrv.setupWebRootPath(_fileRepository->getDirName());
        
        if (!httpSrv.bind(_configuration->getHttpServerPort())) {
            return ErrCode::httpSrvBindError;
        }

        if (!httpSrv.listen(HTTP_SERVER_BACKLOG)) {
            return ErrCode::httpSrvListenError;
        }

        httpSrv.setupLogger(_configuration->verboseModeOn() ? 
            &logger : nullptr);

        return ErrCode::success;
    }

    bool runServer() {
        return  HttpServer::getInstance().run();
    }

    FileRepository::Handle getFileRepositoryHandle() const noexcept {
        return _fileRepository;
    }

    Configuration::Handle getConfiguration() const noexcept {
        return _configuration;
    }
    
private:
    Application() {
    }

    // Parse the command line
    Configuration::Handle _configuration;
    FileRepository::Handle _fileRepository;
    FileRepository::TimeOrderedFileList _timeOrderedFileListCache;
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
        std::clog << Tools::getLocalTime() << std::endl
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

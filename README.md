# httpsrv
Httpsrv is a retailed version of a lightweight HTTP server and derives from thttpd (https://github.com/eantcal/thttpd), originally implemented to serve http GET/HEAD methods. httpsrv has been refactored to support HTTP methods GET and POST.
It has been implemented in modern C++, which means it requires a C++14 or even C++17 compiler to be successfully built.
It has been designed to run on Linux mainly, but it can run also on MacOS or other unix platforms other than Windows. 

httpsrv is capable to serve multiple clients supporting GET and POST methods and has been designed to respond to the following specifications:
- standalone application containing an embedded web server which exposes the following HTTP API for storing and retrieving text files and their associated metadata:
- `POST` `some_file.txt` to `/store`: returns a JSON payload with file metadata containing name, size (in bytes), request timestamp and an auto-generated ID
- `GET` `/files`: returns a JSON payload containing an array of files metadata containing file name, size (in bytes), timestamp and ID
- `GET` `/files/{id}`: returns a JSON payload with file metadata containing name, size (in bytes), timestamp and ID for the provided ID `id`
- `GET` `/files/{id}/zip`: returns a zip archive containing the file which corresponds to the provided ID `id`
- `GET` `/mrufiles`: returns a JSON payload with an array of files metadata containing file name, size (in bytes), timestamp and ID for the top `N` most recently accessed files via the `/files/{id}` and `/files/{id}/zip` endpoints. `N` should be a configurable parameter for this application.
- `GET` `/mrufiles/zip`: returns a zip archive containing the top `N` most recently accessed files via the `/files/{id}` and `/files/{id}/zip` endpoints. `N` should be a configurable parameter for this application.

##httpsrv architecture
Each HTTP sessions runs in a separate thread so multiple requests can be served concurrently. It is guaranteed that independent operations are thread safe. 
The server relies on the filesystem for storing the text files, so it creates (if not already existent) a specific directory that we called ‘file repository’ (class FileRepository). By default the directory is a subdir of user home directory and can be configured at start-up. The default name is “.httpsrv” for ‘unix’ platforms, “httpsrv” for Windows. 
The chance of conflicts and their consequences are the same of trying to access to the same file in a filesystem (because the repository path can be hosted on any supported filesystem, this can have an impact read/write operations) at the same time:
- Concurrent GET operations which not altering the timestamp can be executed without any issue.
- Concurrent POST or GET (id or id/zip) for the same files can fail or produce a status (the JSON descriptor) which does not reflect - for some clients - the actual repository status. Repeating the operation later, in absence of conflicts, will fix the issue.
In absence of specific requirement we decide not to implement a strictly F/S locking mechanism for avoiding conflicts (optimistic solution), which basically allowed to simplify the design.
The id is a SHA256 hash code of file name (which is assumed to be unique), so any conflict has no impact on its validity and a map (FilenameMap instance), which resolves the filename for a given id, is itself thread-safe and locked by r/w mutex. So it is guaranteed that conflicts do not make crashing the server or make it asymptotically unstable.
The application is also designed to recover from intentionally or not intentionally restart (an external process monitor could be used for such purpose).
httpsrv updates the FilenameMap content at start-up (as the http requests are not accepted yet) reading any files present in the configured repository path. This allows the server to restart from a given status in case for some reason must be restarted. 
Multiple instance of httpsrv could be run concurrently on the same system, binding on separate ports. In case they share the same repository, it is not guaranteed that a file posted from a server can be got from another one because the FilenameMap instance lies in each process memory and there was not any requirements for such unsupported scenario.

When a file is uploaded via POST the server does:
- create a HttpServerSession (which executes in a concurrent thread)
- in absence of errors, write the file in a local repository (by default ~/.httpsrv directory)
- update a thread-safe FilenameMap instance
- generates a JSON file descriptor (as per specification)
- in case of error produces a standard HTTP error (40x, 50x depending on the issue)
- responds back to the client (JSON or HTML error report). Because it is not specified how to manage the errors, the decision made is to respond using a standard HTTP error code and a small error description formatted in HTML.

GET operation 


##httpsrv implementation
Httpsrv is a console application (can be easly daemonized via external command like daemonize if required, in Linux, for example). Main thread executes the HTTP server (implemented by HttpServer class).
The HttpServer is instantiated by an Application object (created directly by main() function) which basically is a builder for it. Application verifies and completes the configuration as a mix of default and optional parameters (via program arguments), creates an instance of HttpServer (which is a singleton) and by specific setter methods configures it by setting:
- bind port (default 8080)
- an optional logger object (any std::ostream reference, default std::cout)
- repository path (default ~/.httpsrv).
Application object calls bind/listen methods of HttpServer and in absence of errors finally calls its run() method. This method is blocking for the caller, so unless the application is executed as background process or daemon, it blocks for the caller.
The server binds on any interfaces (including the loopback) and the configured port.


##making httpsrv
Httpsrv derives from thttpd, a lightweight web server I have implemented in C++11 and published at https://github.com/eantcal/thttpd. thttpd provides a basic support for GET/HEAD methods. Thread-model, sockets support, http parsing/formatting support have been reused in part, and refactored for new specifications.

The server relies on C++ standard library (which is part of language) and other few 3pp part libraries such as:
- PicoSHA2 - https://github.com/okdshin/PicoSHA2 (SHA256 hash generator)
- zip - https://github.com/kuba--/zip (a portable simple zip library written in C)
- Boost Filesystem Library Version 3 - (https://www.boost.org/doc/libs/1_67_0/libs/filesystem/doc/index.htm)

PicoSHA2 is a simple alternative to OpenSSL implementation.
Zip is quite simple and portable as well. 
I have embedded the repositories content of such libraries in httpsrv source tree (in 3pp subdir) and included the related source code as part of project, which is the simplest  way to embed them.
Wrapper function/class for such libraries have been provided:
- ZipArchive (ZipArchive.h) is a wrapper class around zip functions
- function hashCode exported by FileUtils is a wrapper around picosha2::hash256_hex_string function
Boost Filesystem can be replaced by standard version if fully supported by C++ compiler ( defining USE_STD_FS).

HTTP protocol has been supported only for providing the specific API exposed.
Low level communications is provided by socket library (WinSocket on Windows), wrapped around following classes:
- TransportSocket and TcpSocket classes expose basic socket functions including send/recv APIs.
- TcpListener provides the interface for passive TCP functions such a listen and accept.
 

Tested on the following platforms:
CMake support and Visual Studio 2019 project files are provided.
The server has been built and tested on Linux, MacOS and Windows, more precisely it has been tested on:
- Darwin (18.7.0 Darwin Kernel Version 18.7.0) on MacBook Pro, built using Apple clang version 11.0.0 (clang-1100.0.20.17), Target: x86_64-apple-darwin18.7.0, Thread model: posix, cmake version 3.12.2
- Linux 5.0.0-38-generic #41-Ubuntu SMP Tue Dec 3 00:27:35 UTC 2019 x86_64 x86_64 x86_64 GNU/Linux, built using g++ (Ubuntu 8.3.0-6ubuntu1) 8.3.0, cmake version 3.13.4
- Windows 10 buillt using Visual Studio 2019

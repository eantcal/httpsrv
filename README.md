# httpsrv
Httpsrv is a retailed version of a lightweight HTTP server and derives from thttpd (https://github.com/eantcal/thttpd), originally implemented to serve http GET/HEAD methods. 
Httpsrv has been implemented in modern C++, which means it requires a C++14 or even C++17 compiler to be successfully built.
It has been designed to run on Linux mainly, but it can also run on MacOS or other Unix platforms other than Windows. 

httpsrv is capable to serve multiple clients supporting GET and POST methods and has been designed to respond to the following specifications:

* standalone application containing an embedded web server which exposes the following HTTP API for storing and retrieving text files and their associated metadata:
* `POST` `some_file.txt` to `/store`: returns a JSON payload with file metadata containing name, size (in bytes), request timestamp and an auto-generated ID
* `GET` `/files`: returns a JSON payload containing an array of files metadata containing file name, size (in bytes), timestamp and ID
* `GET` `/files/{id}`: returns a JSON payload with file metadata containing name, size (in bytes), timestamp and ID for the provided ID `id`
* `GET` `/files/{id}/zip`: returns a zip archive containing the file which corresponds to the provided ID `id`
* `GET` `/mrufiles`: returns a JSON payload with an array of files metadata containing file name, size (in bytes), timestamp and ID for the top `N` most recently accessed files via the `/files/{id}` and `/files/{id}/zip` endpoints. `N` should be a configurable parameter for this application.
* `GET` `/mrufiles/zip`: returns a zip archive containing the top `N` most recently accessed files via the `/files/{id}` and `/files/{id}/zip` endpoints. `N` should be a configurable parameter for this application.

## httpsrv architecture
Httpsrv is a console that can be easily daemonized (via external command like daemonize if required).
Main thread executes the HTTP server implemented by `class HttpServer`.

### Configuration and start-up
Application entry point, the main function, creates an `class Application` which is a builder for `FileRepository` and `HttpServer`:
* class `FileRepository` provides methods for:
    * accessing the filesystem, 
    * creating zip archives,
    * creating a `FilenameMap` object (`filename` resolver for a given `id`), 
    * formatting the JSON metadata
* class `HttpServer` implements the HTTP server main loop:
    * accepting a new TCP connection and 
    * finally creating an HTTP session (`HTTPSession`) which finally handles the application business logic.
    
httpsrv relies on the filesystem for storing files, so it creates in the context `FileRepository` initialization - if not already existent - a specific directory handled by class `FileRepository`. 
The repository directory can be configured via program arguments, and by default it is a subdir of user home directory named `“.httpsrv”` for ‘unix’ platforms, `“httpsrv”` for Windows.

When the initialization is completed, the application object calls the method `run()` of `HttpServer` object. Such method is blocking for the caller, so unless the application is executed as background process or daemon, it will block the caller process (typically the shell).
The server is designed to bind on any interfaces and a configurable TCP port (`8080`, by default).

## Main server loop
The method `HttpServer::run()` executes a loop that for each iteration waits for incoming connection by calling `accept()` which eventually will result in calling the `accept()` function of socket library. 
When `accept()` accepts a connection, `run()` gets a new `TcpSocket` object, which represents the TCP session, and creates a new HTTP Session handled by instance of class `HttpSession`. 
Each HTTP Session runs in a separate thread so multiple requests can be served concurrently. It is guaranteed that independent operations are thread safe. 

### Concurrent operations 
* Concurrent GET operations not altering the timestamp can be executed without any issue.
* Concurrent POST or GET (id or id/zip) for the same files might produce a JSON metadata which does not reflect - for some concurrent clients - the actual repository status. Repeating the operation later, in absence of concurrent access to the same file, will fix the issue (BTW this is what commonly happens in a shared  and unlocked unix filesystem).

In absence of specific requirements it has been decided of not implementing a strictly F/S locking mechanism by adopting an optimistic policy: moreover adding an additional memory r/w lock mechanism would prevent a single instance of httpsrv to access a shared resource, but that would not work for external access to repository itself. Only an effective mechanism relying on a well-orchestrated solution could prevent that.
The file id is a SHA256 hash code of file name (which is in turn assumed to be unique), so any conflict would have no impact on its validity, moreover the class `FilenameMap` (which provides the methods to resolve the filename for a given id) is designed to be thread-safe (a r/w mutex is used for the purposes). So the design should avoid concurrent requests would result in server crash or asymptotic instability.

## Resiliency
The application is also designed to recover from intentional or unintentional restart.
For such reason, httpsrv updates the `FilenameMap` object at start-up, as the http requests are not accepted yet, getting the status of any files present in the configured repository path. This allows the server to restart from a given repository state. 

Multiple instances of httpsrv could be run concurrently on the same system, binding on separate ports. In case they share the same repository, it is not guaranteed that a file posted from a server can be visible to another one because the `FilenameMap` object would be stored in each (isolated) process memory, unless the server is restarted.

### POST
When a file is uploaded via POST API the server does:
* create a HttpSession (which executes in a concurrent thread);
* (in absence of errors) write the file in a local repository (by default `~/.httpsrv`);
* update a thread-safe `FilenameMap` instance;
* generates a JSON file metadata;
* in case of error produce a standard HTTP error (40x, 50x depending on the issue);
* respond back to the client (either via JSON or HTML error report). 

### GET
When get request is :
* create a HttpSession (which executes in a concurrent thread);
* (in absence of errors) write the file in a local repository (by default `~/.httpsrv`);
* update a thread-safe `FilenameMap` instance;
* generates a JSON file metadata;
* in case of error produce a standard HTTP error (40x, 50x depending on the issue);
* respond back to the client (either via JSON or HTML error report). 


### HTTP Errors
httpsrv notifies errors by using a standard HTTP error code and a small error description formatted in HTML.
Empty repository or zero file size is not considered an error. In the first scenario just an empty JSON list `[]` will be send back by server on both `GET` `/files` and `GET` `/mrufiles` valid requests.
If the URI does not respect the given syntax an `HTTP 400 Bad Request` error will be sent to the client. 
If the URI does is valid but the id not found an `HTTP 404 Not Found` error will be sent to the client.
The release version of httpsrv strips out the asserts, so in case of bugs or hardware failures or resources (e. g. memory) exhausted might generate an `HTTP 500 Internal Server Error`.

GET operation 


## httpsrv implementation


## making httpsrv
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
- Windows 10 built using Visual Studio 2019


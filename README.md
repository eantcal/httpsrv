# httpsrv

httpsrv is a retailed version of a lightweight HTTP server and derives from [thttpd](https://github.com/eantcal/thttpd), originally implemented to serve http GET/HEAD methods.

![thttpd](https://github.com/eantcal/thttpd/blob/master/pics/tinyhttp1.png)

httpsrv has been implemented in modern C++, which means it requires a C++14 or even C++17 compiler to be successfully built.
It has been designed to run on Linux, but it can also run on MacOS or other Unix/Posix platforms other than Windows.

httpsrv is capable to serve multiple clients supporting GET and POST methods and has been designed to respond to the following specifications:

* standalone application containing an embedded web server which exposes the following HTTP API for storing and retrieving text files and their associated metadata:
* `POST` `some_file.txt` to `/store`: returns a JSON payload with file metadata containing name, size (in bytes), request timestamp and an auto-generated ID
* `GET` `/files`: returns a JSON payload containing an array of files metadata containing file name, size (in bytes), timestamp and ID
* `GET` `/files/{id}`: returns a JSON payload with file metadata containing name, size (in bytes), timestamp and ID for the provided ID `id`
* `GET` `/files/{id}/zip`: returns a zip archive containing the file which corresponds to the provided ID `id`
* `GET` `/mrufiles`: returns a JSON payload with an array of files metadata containing file name, size (in bytes), timestamp and ID for the top `N` most recently accessed files via the `/files/{id}` and `/files/{id}/zip` endpoints. `N` should be a configurable parameter for this application.
* `GET` `/mrufiles/zip`: returns a zip archive containing the top `N` most recently accessed files via the `/files/{id}` and `/files/{id}/zip` endpoints. `N` should be a configurable parameter for this application.

## httpsrv architecture

httpsrv is a console application (which can be easily daemonized via external command like daemonize, if required).
Its main thread executes the main HTTP server loop. Each client request, a separate HTTP session, is then handled in a specific thread. Once the session terminates, the thread and related resources are freed.

When a client POSTs a new file, that file is stored in a specific `repository` (which is basically a dedicated directory).
`File metadata` is always generated by reading the file attributes, while an id-to-filename thread-safe map (`FilenameMap` instance) is shared among the HTTP sessions (threads), and used to retreive the `filename` for a given `id`. The `id` is generated by an hash code function (which executes the SHA256 algorithm).
When a zip archive is required, that is generated on-the-fly in a temporary directory and sent to the client. Eventually the temporary directory and its content is cleaned up.

### Configuration and start-up

httpsrv entry point, the application `main()` function, creates a `class Application` object which is in turn a builder for classes `FileRepository` and `HttpServer`:

* `FileRepository` provides methods for:
  * accessing the filesystem,
  * creating the repository directory,
  * managing the temporary directories required for sending the zip archives,
  * creating zip archives,
  * creating and handling the `FilenameMap` object
  * formatting the JSON metadata
* `HttpServer` implements the HTTP server main loop:
  * accepting a new TCP connection and
  * finally creating HTTP sessions (`HTTPSession`) where the API business logic is implemented.

The file repository directory (if not already existent) is created in the context `FileRepository` initialization.
The repository directory path can be configured at start-up. By default, it is a subdir of user home directory named `“.httpsrv”` for ‘unix’ platforms, `“httpsrv”` for Windows.

When the initialization is completed, the application object calls the method `run()` of `HttpServer` object. Such method is blocking for the caller, so unless the application is executed as background process or daemon, it will block the caller process (typically the shell).
The server is designed to bind on any interfaces and a specific and configurable TCP port (which is `8080`, by default).

## Main server loop

The method `HttpServer::run()` executes a loop that, for each iteration, waits for incoming connection by calling `accept()` method which will eventually result in calling `accept()` function of socket library.
When `accept()` accepts a connection, `run()` gets a new `TcpSocket` object, which represents the TCP session, and creates a new HTTP Session handled by instance of class `HttpSession`.
Each HTTP Session runs in a separate thread so multiple requests can be served concurrently.

### Concurrent operations by multiple clients

* Concurrent GET operations not altering the timestamp can be executed without any conflicts.
* Concurrent POST or GET (id or id/zip) for the **same** files might produce a JSON metadata which does not reflect - for some concurrent clients - the actual repository status. This is what commonly happens in a shared and unlocked unix filesystem that implements a optimistic non-locking policy. While running on locked filesystem which implements an exclusive access policy, one of the concurrent operation might fail generating an error to the client.
Get operations which require a zip file can be executed cuncurrently without conflicts: the zip file is created in a unique `temporary-directory` whose name is generated randomically, so its integrity is always preserved.
In absence of specific requirements it has been decided not to implement a strictly F/S locking mechanism, adopting indeed an optimistic policy. The rationale is to maintain the design simple, also considering the remote chance of conflicts and their negligible effects.
The file `id` is a hash code of file name (which is in turn assumed to be unique), so any conflicts would have no impact on its validity, moreover the class `FilenameMap` (which provides the methods to resolve the filename for a given id) is designed to be thread-safe (a r/w locking mechanism is used for the purposes). So the design should avoid that concurrent requests resulted in server crash or asymptotic instability.

## Resiliency

The application is also designed to be recover from intentional or unintentional restart.
For such reason, httpsrv updates the `FilenameMap` object at start-up (reading the repository files list), as the http requests are not accepted yet, getting the status of any files present in the configured repository path. This allows the server to restart from a given repository state.

Multiple instances of httpsrv could be run concurrently on the same system, binding on separate ports. In case they share the same repository, it is not guaranteed that a file posted from a server can be visible to another server (until such server executes a request for file list or is restarted) because the `FilenameMap` object would be stored in each (isolated) process memory.

GET and POST requests are processed within a `HTTPSession` loop which consists in:

* reading receiving and parsing an http request (`HttpRequest`)
* validating and classifieng the request
* executing the business logic related to the request (GET/POST)
* creating a response header + a body (`HTTPResponse`)
* cleaning-up resourses (deleting for example the temporary directory of a zip file sent to the client) and ending the session

### POST

When a file is uploaded, the POST business logic consists in:

* (in absence of errors) writing the file in the repository;
* updating a id-filename map (`FilenameMap`);
* generates a JSON file metadata from stored file attribute;
* reply to the client either sending back a JSON metadata or HTTP/HTML error response depending on success or failure of one of previous steps.

### GET

When a get request is processed the business logic performs the following action:

* `/files`: formats a JSON formatted body containing a list of metadata corrisponding to file attributes read by repository
* `/mrufiles`: likewise in `/file`, but the metadata list is generated by a timeordered map and limited to max number of mrufiles configured (3 by default)
* `/files/<id>`:
  * resolves the id via `FilenameMap` object,
  * updates the file timestamp,
  * reads the file attributes,
  * writes in the HTTP response body the JSON metadata reppresenting the file attributes
* `/files/<id>/zip`:
  * resolves the id via `FilenameMap` object,
  * updates the file timestamp,
  * reads the file attributes,
  * creates a zip archive containing the file in a unique temporary directory
  * writes the zip binary in the HTTP response body
  * cleans up the temporary directory
* `/mrufiles/zip`:
  * creates a list of mru files
  * adds each file in a new zip archive stored in unique temporary directory
  * writes the zip binary in the HTTP response body
  * cleans up the temporary directory

### HTTP Errors

httpsrv notifies errors by using a standard HTTP error code and a related error description, formatted in HTML body.
Empty repository or zero file size is not considered an error. In the first scenario just an empty JSON list `[]` will be sent back by server on both `GET` `/files` and `GET` `/mrufiles` valid requests.
If the URI does not respect the given syntax, an `HTTP 400 Bad Request` error will be sent to the client.
If the URI does is valid but the id not found, an `HTTP 404 Not Found` error will be sent to the client.
Building a `release` version of httpsrv binary strips out assert() calls, so in case of bugs or hardware failures or resources (e.g. memory) exhausted that might generate an `HTTP 500 Internal Server Error`.

## Low level communication

Low level communication is provided by socket library (WinSocket on Windows), wrapped by following classes:

* `TransportSocket` and `TcpSocket` classes expose basic socket functions including `send/recv` APIs.
* `TcpListener` provides a wrapper of some passive TCP functions such `listen` and `accept`.
* `HTTPSocket` provides parsing and formatting methods for supported HTTP protocol features.

## Timestamp and precision

File timestamp is a GMT representation of last access attribute of a file (as documented in `stat` syscall). Some filesystem could not support microsecond field, so the timestamp can result rounded to the second.

## Other utilities

A miscellanea of additional functions/classes have been implemented to provide some utilities handling date/time, strings, files, JSON formatting, etc.

## 3pp

The server relies on C++ standard library (which is part of language) and other few 3pp part libraries such as:

* [PicoSHA2](https://github.com/okdshin/PicoSHA2), single header file SHA256 hash generator)
* [zip](https://github.com/kuba--/zip), a portable simple zip library written in C)
* [Boost Filesystem Library Version 3](https://www.boost.org/doc/libs/1_67_0/libs/filesystem/doc/index.htm)

Source code of such libraries (except Boost) has been copied in httpsrv source tree in 3pp subdir and included the related source code as part of project in the CMakeLists.txt or VS project file.

Wrapper function/class for such libraries have been provided:

* class `ZipArchive` is a wrapper on employed `zip` functions
* `hashCode()` function part of `FileUtils.h` is a wrapper for `picosha2::hash256_hex_string` function
Boost Filesystem can be replaced by C++ standard version if fully supported by C++ compiler (defining the preprocessor symbol `USE_STD_FS` at compile time).

## Limitations

HTTP protocol has been supported only for providing the specific API exposed.
Timestamp precision of some filesystem implementation might not support the microseconds field.

## Tested Platforms

Tested on the following platforms:
CMake support and Visual Studio 2019 project files are provided.
The server has been built and tested on Linux, MacOS and Windows, more precisely it has been tested on:

* Darwin (18.7.0 Darwin Kernel Version 18.7.0) on MacBook Pro, built using Apple clang version 11.0.0 (clang-1100.0.20.17), Target: x86_64-apple-darwin18.7.0, Thread model: posix, cmake version 3.12.2
* Linux 5.0.0-38-generic #41-Ubuntu SMP Tue Dec 3 00:27:35 UTC 2019 x86_64 x86_64 x86_64 GNU/Linux, built using g++ (Ubuntu 8.3.0-6ubuntu1) 8.3.0, cmake version 3.13.4
* Windows 10 built using Visual Studio 2019

to be continued...

//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */
// HTTP Server

/* -------------------------------------------------------------------------- */

#include "HttpSession.h"
#include "HttpSocket.h"

#include <cassert>

/* -------------------------------------------------------------------------- */
// HttpSession

/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */

void HttpSession::logSessionBegin()
{
   if (!_verboseModeOn)
      return;

   const int sd = getTcpSocketHandle()->getSocketFd();
   _sessionId =
      "[" + std::to_string(sd) + "] " +
      "[" + SysUtils::getUtcTime() + "] ";

   log() << _sessionId << "---- HTTP SERVER SESSION STARTS" << std::endl;
   log().flush();
}

void HttpSession::logEnd()
{
   if (!_verboseModeOn)
      return;

   log() << _sessionId << "---- HTTP SERVER SESSION ENDS" << std::endl
      << std::endl;
   log().flush();
}

//! Process HTTP GET Method
HttpSession::processAction HttpSession::processGetRequest(
   HttpRequest& incomingRequest,
   std::string& json,
   std::string& nameOfFileToSend,
   FileUtils::DirectoryRipper::Handle& zipCleaner)
{
   const auto& uri = incomingRequest.getUri();

   // command /files
   if (uri == HTTPSRV_GET_FILES &&
      _FileRepository->getFilenameMap().
      locked_updateMakeJson(getLocalStorePath(), json))
   {
      return processAction::sendJsonFileList;
   }

   // command /mrufiles
   if (uri == HTTPSRV_GET_MRUFILES)
   {
      return !_FileRepository->createJsonMruFilesList(json) ?
         processAction::sendInternalError :
         processAction::sendMruFiles;
   }

   // command /mrufiles/zip
   if (uri == HTTPSRV_GET_MRUFILES_ZIP)
   {
      return _FileRepository->createMruFilesZip(nameOfFileToSend, zipCleaner) ?
         processAction::sendZipFile :
         processAction::sendInternalError;
   }

   const auto& uriArgs = incomingRequest.getUriArgs();

   // command /files/<id> is split in 3 args (first one, arg[0] is dummy)
   if (uriArgs.size() == 3 && uriArgs[1] == HTTP_URIPFX_FILES)
   {
      const auto& id = incomingRequest.getUriArgs()[2];
      if (!_FileRepository->getFilenameMap().
         jsonStatFileUpdateTS(getLocalStorePath(), id, json, true))
      {
         return processAction::sendInternalError;
      }
   }

   // command /files/<id>/zip is split in 4 args (first one, arg[0] is dummy)
   if (uriArgs.size() == 4 &&
      uriArgs[1] == HTTP_URIPFX_FILES &&
      uriArgs[3] == HTTP_URISFX_ZIP)
   {
      const auto& id = uriArgs[2];
      const auto res = _FileRepository->createFileZip(id, nameOfFileToSend, zipCleaner);
      switch (res)
      {
      case FileRepository::createFileZipRes::idNotFound:
         return processAction::sendNotFound;

      case FileRepository::createFileZipRes::cantCreateTmpDir:
      case FileRepository::createFileZipRes::cantZipFile:
         return processAction::sendInternalError;

      case FileRepository::createFileZipRes::success:
         return processAction::sendZipFile;
      }
   }

   return processAction::sendErrorInvalidRequest;
}

/* -------------------------------------------------------------------------- */

//! Process HTTP POST method
void HttpSession::processPostRequest(
   HttpRequest& incomingRequest, 
   std::string& jsonResponse)
{
   const auto& fileName = incomingRequest.getFileName();

   if (_verboseModeOn)
   {
      log() << _sessionId << "Writing '" << fileName << "'" << std::endl;
      log().flush();
   }

   if (!_FileRepository->store(fileName, incomingRequest.getBody(), jsonResponse))
   {
      if (_verboseModeOn)
      {
         log() << _sessionId << "Error writing '" << fileName << "'" << std::endl;
         log().flush();
      }
   }
}

/* -------------------------------------------------------------------------- */

// Handles the HTTP server request
// This method executes in a specific thread context for
// each accepted HTTP request
void HttpSession::operator()(Handle taskHandle)
{
   (void)taskHandle;

   logSessionBegin();

   HttpRequest::Handle incomingRequest(new (std::nothrow) HttpRequest);

   assert(incomingRequest);
   if (!incomingRequest) // out-of-memory?
      return;

   while (getTcpSocketHandle())
   {
      // Create an http socket around a connected tcp socket
      HttpSocket httpSocket(getTcpSocketHandle());

      httpSocket >> incomingRequest;

      // If an error occoured terminate the task
      if (!httpSocket)
         break;

      // Log the request
      if (_verboseModeOn)
         incomingRequest->dump(log(), _sessionId);

      std::string jsonResponse;
      std::string nameOfFileToSend;
      processAction action = processAction::none;

      HttpResponse::Handle outgoingResponse;

      // if assigned with non-null DirectoryRipper Handle (a shared pointer)
      // on session termination the DirectoryRipper will eventually clean up the
      // temporary directory and its content created for the zip archive
      // required by GET files/<id>/zip or GET mrufiles/zip operations
      FileUtils::DirectoryRipper::Handle zipCleaner;

      // if this is a pending POST-request containing 'Expected: 100-Continue'
      if (incomingRequest->isExpected_100_Continue_Response() ||
         // or it is not, then checks if incoming request is
         // a valid POST request 
         incomingRequest->isValidPostRequest())
      {
         processPostRequest(*incomingRequest, jsonResponse);
      }

      // else checks if incomingRequest is valid GET request
      else if (incomingRequest->isValidGetRequest())
      {
         action = processGetRequest(
            *incomingRequest,
            jsonResponse,
            nameOfFileToSend,
            zipCleaner);
      }

      // None of above -> respond 400 - Bad Request to the client
      else
      {
         outgoingResponse = std::make_unique<HttpResponse>(400); // Bad Request
      }

      if (!outgoingResponse)
      {
         // Format a response to previous HTTP client request
         outgoingResponse = std::make_unique<HttpResponse>(
            *incomingRequest,
            jsonResponse,
            jsonResponse.empty() ? "" : ".json",
            nameOfFileToSend);
      }

      assert(outgoingResponse);
      if (!outgoingResponse)
         break;

      // Send the response header and any not empty json content to remote peer
      httpSocket << *outgoingResponse;

      // Any binary content is sent following the HTTP response header
      // already sent to the client
      if (action == processAction::sendZipFile)
      {
         if (0 > httpSocket.sendFile(nameOfFileToSend))
         {
            if (_verboseModeOn)
            {
               log() << _sessionId << "Error sending '" << nameOfFileToSend
                  << "'" << std::endl
                  << std::endl;

               log().flush();
            }
            break;
         }
      }

      if (_verboseModeOn)
         outgoingResponse->dump(log(), _sessionId);

      // If we have generated an error, close the session
      if (outgoingResponse->isErrorResponse())
         break;

      if (!incomingRequest->isExpected_100_Continue_Response())
      {
         incomingRequest.reset(new (std::nothrow) HttpRequest);
         assert(incomingRequest);
         if (!incomingRequest)
            break;
      }
      else
      {
         incomingRequest->clearExpectedContinueFlag();
      }

      // After sent a file we can close the HTTP Session
      if (action == processAction::sendZipFile)
         break;
   }

   getTcpSocketHandle()->shutdown();

   logEnd();
}

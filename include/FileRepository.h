//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __FILE_REPOSITORY_H__
#define __FILE_REPOSITORY_H__

#include "FileUtils.h"
#include "FilenameMap.h"

#include <map>
#include <list>
#include <memory>
#include <cassert>

/* -------------------------------------------------------------------------- */

//! Helper class to handle the server local repository for uploading files
//! It provides a method to get a JSON formatted list of getMruFilesN()
//! mru files.
class FileRepository
{

public:
   using Handle = std::shared_ptr<FileRepository>;

   /**
     * Creates a new FileRepository object and a related directory
     * for a given path.
     * @param path is the directory path
     * @param mruFilesN is a number N of max mrufiles built by
     *       createJsonMruFilesList() method
     */
   static Handle make(const std::string& path, int mrufilesN)
   {
      // Creates or validates (if already existant) a repository for text file
      Handle ret(new (std::nothrow) FileRepository(path, mrufilesN));

      assert(ret);

      return ret && ret->init() ? ret : nullptr;
   }

   /**
    * Creates a JSON formatted MRU files list
    * @param json containing the mru files list
    * @return true if operation succeded, false otherwise
    */
   bool createJsonMruFilesList(std::string& json);

   /**
    * Creates a list of mru filenames
    * @param mrufiles containing the list
    * @return true if operation succeded, false otherwise
    */
   bool createMruFilesList(std::list<std::string>& mrufiles);

   /**
    * Returns the repository path
    */
   const std::string& getPath() const noexcept
   {
      return _path;
   }

   /**
    * Returns max number of mru files formatted
    * in json output of createJsonMruFilesList
    */
   int getMruFilesN() const noexcept
   {
      return _mrufilesN;
   }

   /**
    * Returns a FilenameMap reference
    */
   FilenameMap& getFilenameMap() 
   {
      return _filenameMap;
   }

   /**
    * Store a file on the repository.
    *
    * @param fileName is name of file to be stored
    * @param fileContent is the content of file (can be empty)
    * @param json is JSON formatted returned status
    * @return true if operation succeded, false otherwise
    */
   bool store(
      const std::string& fileName,
      const std::string& fileContent,
      std::string& json);

   /**
    * Create a zip archive containing MRU files of repository
    *
    * @param zipFileName is name of file created
    * @param zipCleaner will receive a copy of zipCleaner which
    *        eventually will destroy the temporary zip file
    * @return true if operation succeded, false otherwise
    */
   bool createMruFilesZip(
      std::string& zipFileName,
      FileUtils::DirectoryRipper::Handle& zipCleaner);

   enum class createFileZipRes {
      success,
      idNotFound,
      cantCreateTmpDir,
      cantZipFile
   };

   /**
    * Create a zip archive containing a specific file of repository
    *
    * @paran id is identifier of file
    * @param zipFileName is name of file created
    * @param zipCleaner will receive a copy of zipCleaner which 
    *        eventually will destroy the temporary zip file
    * @return one of possible error code defined in createFileZipRes
    */
   createFileZipRes createFileZip(
      const std::string id, 
      std::string& zipFileName, 
      FileUtils::DirectoryRipper::Handle& zipCleaner);

private:
   FileRepository(const std::string& path, int mrufilesN) :
      _path(path),
      _mrufilesN(mrufilesN)
   {
   }

#ifdef WIN32
   using TimeOrderedFileList = std::multimap<std::time_t, fs::path>;
#else
   using TimeOrderedFileList = std::multimap<long, fs::path>;
#endif

   bool init();
   bool createTimeOrderedFilesList(TimeOrderedFileList& list);

private:
   std::string _path;
   int _mrufilesN;
   FilenameMap _filenameMap;
};

/* ------------------------------------------------------------------------- */

#endif // !__FILE_REPOSITORY_H__

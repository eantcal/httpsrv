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
#include <memory>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

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
   static Handle make(const std::string &path, int mrufilesN)
   {
      Handle ret(new (std::nothrow) FileRepository(path, mrufilesN));
      assert(ret);

      if (!ret)
         return nullptr;

      return ret->init() ? ret : nullptr;
   }

   /**
    * Creates a JSON formatted MRU files list
    * @param json containing the mru files list
    * @return true if operation succeded, false otherwise
    */
   bool createJsonMruFilesList(std::string &json);

   /**
    * Creates a list of mru filenames
    * @param mrufiles containing the list
    * @return true if operation succeded, false otherwise
    */
   bool createMruFilesList(std::list<std::string> &mrufiles);

   /**
    * Returns the repository path
    */
   const std::string &getPath() const noexcept
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

private:
   FileRepository(const std::string &path, int mrufilesN) : 
      _path(path),
      _mrufilesN(mrufilesN)
   {
   }

   using TimeOrderedFileList = std::multimap<std::time_t, fs::path>;

   bool init();
   bool createTimeOrderedFilesList(TimeOrderedFileList &list);

private:
   std::string _path;
   int _mrufilesN;
};

/* ------------------------------------------------------------------------- */

#endif // !__FILE_REPOSITORY_H__

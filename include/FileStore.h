//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __FILE_STORE_H__
#define __FILE_STORE_H__

#include "FileUtils.h"
#include "FilenameMap.h"

#include <map>
#include <memory>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


/* -------------------------------------------------------------------------- */

class FileStore {

public:
   using TimeOrderedFileList = std::multimap<std::time_t, fs::path>;
   using Handle = std::shared_ptr<FileStore>;

   static Handle make(const std::string& path, int mrufilesN) {
      Handle ret(new (std::nothrow) FileStore(path, mrufilesN));
      if (ret) {
         if (!ret->init()) {
            return nullptr;
         }
      }
      return ret;
   }

   bool createJsonMruFilesList(std::string& json);

   const std::string& getPath() const noexcept {
      return _path;
   }

   int getMruFilesN() const noexcept {
      return _mrufilesN;
   }

   
private:
   FileStore(const std::string& path, int mrufilesN) : 
      _path(path),
      _mrufilesN(mrufilesN)
   {}

   bool init();

   bool createMruFilesList(TimeOrderedFileList& list);

private:
   std::string _path;
   int _mrufilesN;
};


/* ------------------------------------------------------------------------- */

#endif // !__FILE_STORE_H__
//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "FilenameMap.h"
#include "FileUtils.h"


/* -------------------------------------------------------------------------- */

bool FilenameMap::scan(const std::string& path)
{
   fs::path dirPath(path);
   fs::directory_iterator endIt;

   if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
      for (fs::directory_iterator it(dirPath); it != endIt; ++it) {
         if (fs::is_regular_file(it->status())) {
            std::string jsonEntry;
            auto fName = it->path().filename().string();
            auto id = FileUtils::hashCode(fName);
            insert(id, fName);
         }
      }
      return true;
   }

   return false;
}


/* -------------------------------------------------------------------------- */

bool FilenameMap::locked_updateMakeJson(
   const std::string& path,
   std::string& json)
{
   fs::path dirPath(path);
   fs::directory_iterator endIt;

   FilenameMap newCache;

   json = "[\n";

   if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
      for (fs::directory_iterator it(dirPath); it != endIt; ++it) {
         if (fs::is_regular_file(it->status())) {
            std::string jsonEntry;
            auto fName = it->path().filename().string();
            auto id = FileUtils::hashCode(fName);
            if (FileUtils::jsonStat(it->path().string(), fName, id, jsonEntry, "  ", ",\n")) {
               newCache.insert(id, fName);
               json += jsonEntry;
            }
         }
      }
      locked_replace(newCache);

      // remove last ",\n" sequence
      if (json.size() > 2)
         json.resize(json.size() - 2);

      json += "\n]\n";
      return true;
   }

   json.clear();
   return false;
}
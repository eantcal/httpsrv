//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "FileRepository.h"
#include "StrUtils.h"

/* -------------------------------------------------------------------------- */

bool FileRepository::init()
{
   std::string repositoryPath;

   // Resolve any homedir prefix
   std::string resPath;

   if (!_path.empty())
   {
      size_t prefixSize = _path.size() > 1 ? 2 : 1;
      if ((prefixSize > 1 && _path.substr(0, 2) == "~/") || _path == "~")
      {
         resPath = FileUtils::getHomeDir();
         resPath += "/";
         resPath += _path.substr(prefixSize, _path.size() - prefixSize);
      }
      else
      {
         resPath = _path;
      }
   }

   if (!FileUtils::touchDir(resPath, repositoryPath))
      return false;

   try
   {
      fs::path cPath(repositoryPath);
      _path = fs::canonical(cPath).string();
   }
   catch (...)
   {
      return false;
   }

   return true;
}

/* -------------------------------------------------------------------------- */

bool FileRepository::createTimeOrderedFilesList(TimeOrderedFileList &list)
{
   fs::path dirPath(getPath());
   fs::directory_iterator endIt;

   if (fs::exists(dirPath) && fs::is_directory(dirPath))
   {
      list.clear();
      for (fs::directory_iterator it(dirPath); it != endIt; ++it)
      {
         if (fs::is_regular_file(it->status()))
            list.insert({fs::last_write_time(it->path()), *it});
      }
      return true;
   }

   return false;
}

/* -------------------------------------------------------------------------- */

bool FileRepository::createMruFilesList(std::list<std::string> &mrufiles)
{
   TimeOrderedFileList timeOrderedFileList;

   if (!createTimeOrderedFilesList(timeOrderedFileList))
      return false;

   int fileCnt = 0;

   for (auto it = timeOrderedFileList.rbegin();
        it != timeOrderedFileList.rend() && fileCnt < _mrufilesN;
        ++it,
             ++fileCnt)
   {
      mrufiles.push_back(it->second.filename().string());
   }

   return true;
}

/* -------------------------------------------------------------------------- */

bool FileRepository::createJsonMruFilesList(std::string &json)
{
   TimeOrderedFileList timeOrderedFileList;

   if (!createTimeOrderedFilesList(timeOrderedFileList))
      return false;

   json = "[\n";

   int fileCnt = 0;

   for (auto it = timeOrderedFileList.rbegin();
        it != timeOrderedFileList.rend() && fileCnt < _mrufilesN;
        ++it,
             ++fileCnt)
   {
      std::string jsonEntry;
      auto fName = it->second.filename().string();
      auto id = FileUtils::hashCode(fName);
      if (FilenameMap::jsonStat(it->second.string(), fName, id, jsonEntry, "  ", ",\n"))
      {
         json += jsonEntry;
      }
   }

   // remove last ",\n" sequence
   if (json.size() > 2)
      json.resize(json.size() - 2);

   json += "\n]\n";
   return true;
}

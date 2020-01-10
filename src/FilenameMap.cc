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

#include <sstream>
#include <iomanip>

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */

bool FilenameMap::scan(const std::string &path)
{
   fs::path dirPath(path);
   fs::directory_iterator endIt;

   if (fs::exists(dirPath) && fs::is_directory(dirPath))
   {
      for (fs::directory_iterator it(dirPath); it != endIt; ++it)
      {
         if (fs::is_regular_file(it->status()))
         {
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
    const std::string &path,
    std::string &json)
{
   fs::path dirPath(path);
   fs::directory_iterator endIt;

   FilenameMap newCache;

   json = "[\n";

   if (fs::exists(dirPath) && fs::is_directory(dirPath))
   {
      for (fs::directory_iterator it(dirPath); it != endIt; ++it)
      {
         if (fs::is_regular_file(it->status()))
         {
            std::string jsonEntry;
            auto fName = it->path().filename().string();
            auto id = FileUtils::hashCode(fName);
            if (FilenameMap::jsonStat(it->path().string(), fName, id, jsonEntry, "  ", ",\n"))
            {
               newCache.insert(id, fName);
               json += jsonEntry;
            }
         }
      }
      locked_replace(std::move(newCache));

      // remove last ",\n" sequence
      if (json.size() > 2)
         json.resize(json.size() - 2);

      json += "\n]\n";
      return true;
   }

   json.clear();
   return false;
}

/* -------------------------------------------------------------------------- */

bool FilenameMap::jsonStatFileUpdateTS(
    const std::string &path,
    const std::string &id,
    std::string &json,
    bool updateTimeStamp)
{
   std::string fName;

   if (!locked_search(id, fName))
      return false;

   std::string filePath = path + "/" + fName;

   if (updateTimeStamp)
      FileUtils::touch(filePath, false /*== do not create if it does not exist*/);

   return jsonStat(filePath, fName, id, json);
}

/* -------------------------------------------------------------------------- */

bool FilenameMap::jsonStat(
    const std::string &filePath, // actual file path (including name)
    const std::string &fileName, // filename field of JSON output
    const std::string &id,       // id field of JSON output
    std::string &jsonOutput,
    const std::string &beginl,
    const std::string &endl)
{
   struct stat rstat = {0};
   const int ret = stat(filePath.c_str(), &rstat);

   if (ret < 0)
      return false;

   // convert into UTC timestamp
   std::tm bt = *gmtime(&(rstat.st_atime));

   std::ostringstream ossTS;

#ifdef __linux__
   // Supported by Linux only
   size_t microsec = (rstat.st_atim.tv_nsec / 1000) % 1000000;
#else
   size_t microsec = 0;
#endif
   // YYYY-MM-DDTHH:MM:SS.uuuuuuZ
   ossTS << std::put_time(&bt, "%Y-%m-%dT%H:%M:%S");
   ossTS << '.' << std::setfill('0') << std::setw(6) << microsec << "Z";

   /*
   Example of JSON output

   {
     "id": "0d0dad8f655e69a1c5788682781bcc143fc9bf55e0b3dbb778e4a85f8e9e586b",
     "name": "nino.txt",
     "size": 123,
     "timestamp": "2020-01-01T17:40:46.560645Z"
   }
   */

   std::stringstream oss;
   oss << beginl << "{" << std::endl;
   // we don't need to escape the is while does contain any control/puntuactor char by contract
   oss << beginl << "  \"id\": \"" << id << "\"," << std::endl;
   // while we need to escape the filename otherwhise resulting JSON could be invalid
   oss << beginl << "  \"name\": \"" << StrUtils::escapeJson(fileName) << "\"," << std::endl;
   oss << beginl << "  \"size\": " << rstat.st_size << "," << std::endl;
   oss << beginl << "  \"timestamp\": \"" << ossTS.str() << "\"" << std::endl;
   oss << beginl << "}" << endl;
   jsonOutput = oss.str();

   return true;
}

//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __ID_FILENAME_MAP_H__
#define __ID_FILENAME_MAP_H__

/* -------------------------------------------------------------------------- */

#include <unordered_map>
#include <mutex> // For std::unique_lock
#include <shared_mutex>
#include <string>

/* -------------------------------------------------------------------------- */

/**
 * Thread-safe id/filename map used to resolve a filename
 * for a given id
 */
class FilenameMap
{
public:
   FilenameMap() = default;

   FilenameMap(const FilenameMap &) = delete;
   FilenameMap(FilenameMap &&) = delete;
   FilenameMap &operator=(const FilenameMap &) = delete;
   FilenameMap &operator=(FilenameMap &&) = delete;

   /**
    * Thread-safe version of insert (thread-safe)
    */
   void locked_insert(const std::string id, const std::string &fileName)
   {
      std::unique_lock lock(_mtx);
      insert(id, fileName);
   }

   /**
    * Inserts <id, filename> in the map
    *
    * @param id is the map key
    * @param fileName is the value
    */
   void insert(const std::string id, const std::string &fileName)
   {
      _data.insert({id, fileName});
   }

   /**
    * Clears the map content (thread-save)
    */
   void clear()
   {
      std::unique_lock lock(_mtx);
      _data.clear();
   }

   /**
    * Replaces the entire map content (thread-safe) with
    * with a given map content.
    *
    * @param newMap is the new map
    */
   void locked_replace(FilenameMap &&newMap)
   {
      std::unique_lock lock(_mtx);
      _data = std::move(newMap._data);
   }

   /**
    * Searches a filename related to a given id (thread-safe)
    *
    * @param id searched id
    * @param fileName is assigned with corrispondent filename if found
    * @return true if id is found, false otherwise
    */
   bool locked_search(const std::string &id, std::string &fileName) const
   {
      std::shared_lock lock(_mtx);

      auto it = _data.find(id);
      if (it != _data.end())
      {
         fileName = it->second;
         return true;
      }
      return false;
   }

   /**
    * Scans the file system path to populate the map
    *
    * @param path of local repository
    * @return true if operation successfully completed, false otherwise
    */
   bool scan(const std::string &path);

   /**
    * Scans the repository to update a given filenameMap
    *
    * @param path of repository
    * @param json is the JSON formatted text matching the cache content
    * @return true if operation successfully completed, false otherwise
    */
   bool locked_updateMakeJson(const std::string &path, std::string &json);

   /**
     * Returns file attributes of fileName formatted using a JSON record of
     * id, name, size, timestame, where id is sha256 of fileName,
     * name is the fileName, size is the size in bytes of file,
     * timestamp is a unix access timestamp of file
     *
     * @param filePath String containing complete file path and name
     * @param fileName String containing the name to generate JSON output
     * @param jsonOutput output JSON string
     * @return true if operation successfully completed, false otherwise
     */
   static bool jsonStat(
       const std::string &filePath,
       const std::string &fileName,
       const std::string &id,
       std::string &jsonOutput,
       const std::string &beginl = "",
       const std::string &endl = "\n");

   /**
    * Touches an existing file and returns a related stat in JSON format
    * @param path points directory where the file is located
    * @param id of file
    * @param jsonOutput output JSON string
    * @return true if operation successfully completed, false otherwise
    */
   bool jsonStatFileUpdateTS(
       const std::string &path,
       const std::string &id,
       std::string &json,
       bool updateTimeStamp);
private:
   using data_t = std::unordered_map<std::string, std::string>;
   mutable std::shared_mutex _mtx;
   data_t _data;
};

/* -------------------------------------------------------------------------- */

#endif // !__ID_FILENAME_MAP_H__
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
#include <mutex>  // For std::unique_lock
#include <shared_mutex>
#include <string>
#include <memory>


/* -------------------------------------------------------------------------- */

/**
 * Thread-safe id/filename map used to resolve a filename
 * for a given id
 */
class FilenameMap {
public:
   using Handle = std::shared_ptr<FilenameMap>;

   FilenameMap(const FilenameMap&) = delete;
   FilenameMap(FilenameMap&&) = delete;
   FilenameMap& operator=(const FilenameMap&) = delete;
   FilenameMap& operator=(FilenameMap&&) = delete;

   /**
    * Create an object instance
    */
   static Handle make() {
      return FilenameMap::Handle(new (std::nothrow) FilenameMap);
   }

   /**
    * Thread-safe version of insert
    */
   void locked_insert(const std::string id, const std::string& fileName) {
      std::unique_lock lock(_mtx);
      insert( id, fileName );
   }

   /**
    * Insert <id, filename> in the map
    * @param id is the map key
    * @param fileName is the value
    */
   void insert(const std::string id, const std::string& fileName) {
      _data.insert({ id, fileName });
   }

   /**
    * Clear the cache content
    */
   void clear() {
      std::unique_lock lock(_mtx);
      _data.clear();
   }

   /**
    * Replace the entire cache content
    */
   void locked_replace(FilenameMap& newCache) {
      std::unique_lock lock(_mtx);
      _data = std::move(newCache._data);
   }

   /**
    * Search a filename related to a given id
    * @param id searched id
    * @param fileName is assigned with corrispondent filename if found
    * @return true if id is found, false otherwise
    */
   bool locked_search(const std::string& id, std::string& fileName) const {
      std::shared_lock lock(_mtx);

      auto it = _data.find(id);
      if (it != _data.end()) {
         fileName = it->second;
         return true;
      }
      return false;
   }

   FilenameMap() = default;

private:
   using data_t = std::unordered_map<std::string, std::string>;
   mutable std::shared_mutex _mtx;
   data_t _data;
};


/* -------------------------------------------------------------------------- */

#endif // !__ID_FILENAME_MAP_H__
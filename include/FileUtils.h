//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__


/* -------------------------------------------------------------------------- */

#include "StrUtils.h"
#include "FilenameMap.h"

#include <map>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


/* -------------------------------------------------------------------------- */

//! @brief Collection of some f/s utility functions
namespace FileUtils {

   /**
    *  Creates a temporary directory
    *  @return true if operation successfully completed, false otherwise
    */
   bool createTemporaryDir(fs::path& path);


   /**
     * Returns file attributes of fileName.
     *
     * @param fileName String containing the path of existing file
     * @param dateTime is updated with the file timestamp
     * @param ext is updated with File extension or "." if there is no any
     * @return true if operation successfully completed, false otherwise
     */
   bool fileStat(
      const std::string& fileName, 
      std::string& dateTime,
      std::string& ext, 
      size_t& fsize);


   /**
     * Updates a file timestamp to now
     *
     * @param fileName String containing the path of existing or new file
     * @param createNewIfNotExists is optional parameter, when true force
       *      to create a new file if not already existant
     * @return true if operation successfully completed, false otherwise
     */
   bool touch(const std::string& fileName, bool createNewIfNotExists=false);


   /**
     * Gets full path of existing file or directory
     *
     * @param partialPath String containing the path of existing file
     * @param fullPath String containing the full path of existing file
     * @return true if operation successfully completed, false otherwise
     */
   bool getFullPath(const std::string& partialPath, std::string& fullPath);


   /**
     * Returns true if pathName exists and it is an accessable directory
     *
     * @param pathName String containing a path of directory
     * @return true if directory found, false otherwise
     */
   bool directoryExists(const std::string& pathName);


   /**
     * Creates a new relative directory relativeDirName if not already existant
     * and retreives the full path name
     *
     * @param relativeDirName String containing a path of directory
     * @param fullPath String that will contain the full path of the directory
     * @return true if directory found, false otherwise
     */
   bool touchDir(const std::string& relativeDirName, std::string& fullPath);


   /**
     * Returns user's home directory
     *
     * @return string containig user home directory path
     */
   std::string getHomeDir();


   /**
     * Creates an unique id from a given string
     *
     * @param src Source string
     * @return id
     */
    std::string hashCode(const std::string& src);
};


/* ------------------------------------------------------------------------- */

#endif // !__FILE_UTILS_H__

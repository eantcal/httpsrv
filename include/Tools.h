//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __TOOLS_H__
#define __TOOLS_H__


/* -------------------------------------------------------------------------- */

#include <regex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "OsSpecific.h"


/* -------------------------------------------------------------------------- */

namespace Tools {


/* -------------------------------------------------------------------------- */

using TimeoutInterval = std::chrono::system_clock::duration;


/* -------------------------------------------------------------------------- */

/**
 * Converts a timeval object into standard duration object
 *
 * @param d  Duration
 * @param tv Timeval source object
 */
void convertDurationInTimeval(const TimeoutInterval& d, timeval& tv);


/* -------------------------------------------------------------------------- */

/**
 * Gets the system time, corrected for the local time zone
 * Time format is "DoW Mon dd hh:mm:ss yyyy"
 * Example "Thu Sep 19 10:03:50 2013"
 *
 * @param localTime will contain the time
 */
void getLocalTime(std::string& localTime);


/* -------------------------------------------------------------------------- */

/**
 * Gets the system time, corrected for the local time zone
 * Time format is "DoW Mon dd hh:mm:ss yyyy"
 * Example "Thu Sep 19 10:03:50 2013"
 *
 * @return the string will contain the time
 */
inline std::string getLocalTime()
{
    std::string lt;
    getLocalTime(lt);
    return lt;
}


/* -------------------------------------------------------------------------- */

/**
 *  Removes any instances of character c if present at the end of the string s
 *
 *  @param s The input / ouput string
 *  @param c Searching character to remove
 */
void removeLastCharIf(std::string& s, char c);


/* -------------------------------------------------------------------------- */

/**
 * Returns file attributes of fileName.
 *
 * @param fileName String containing the path of existing file
 * @param dateTime Time of last modification of file
 * @param ext File extension or "." if there is no any
 * @return true if operation successfully completed, false otherwise
 */
bool fileStat(const std::string& fileName, std::string& dateTime,
    std::string& ext, size_t& fsize);


/* -------------------------------------------------------------------------- */

/**
 * Create an unique id from a given string
 *
 * @param src Source string
 * @return id
 */
std::string hashCode(const std::string& src);


/* -------------------------------------------------------------------------- */

/**
 * Returns file attributes of fileName formatted using a JSON record of
 * id, name, size, timestame, where id is sha256 of fileName,
 * name is the fileName, size is the size in bytes of file, 
 * timestamp is a unix access timestamp of file
 *
 * @param fileName String containing the path of existing file
 * @param jsonOutput output JSON string
 * @return true if operation successfully completed, false otherwise
 */
bool jsonStat(const std::string& fileName, std::string& jsonOutput);


/* -------------------------------------------------------------------------- */

/**
 * Trivial but portable version of basic touch command
 *
 * @param fileName String containing the path of existing or new file
 * @return true if operation successfully completed, false otherwise
 */

bool touch(const std::string& fileName);


/* -------------------------------------------------------------------------- */

/**
 * Gets full path of existing file or directory
 *
 * @param partialPath String containing the path of existing file
 * @param fullPath String containing the full path of existing file
 * @return true if operation successfully completed, false otherwise
 */

bool getFullPath(const std::string& partialPath, std::string& fullPath);


/* -------------------------------------------------------------------------- */

/**
 * Return true if pathName exists and it is an accessable directory
 *
 * @param pathName String containing a path of directory
 * @return true if directory found, false otherwise
 */
bool directoryExists(const std::string& pathName);


/* -------------------------------------------------------------------------- */

/**
 * Creates a new relative directory relativeDirName if not already existant 
 * and retreives the full path name
 *
 * @param relativeDirName String containing a path of directory
 * @param fullPath String that will contain the full path of the directory
 * @return true if directory found, false otherwise
 */
bool touchDir(const std::string& relativeDirName, std::string& fullPath);


/* -------------------------------------------------------------------------- */

/**
 * Returns user's home directory
 *
 * @return string containig user home directory path
 */
std::string getHomeDir();


/* -------------------------------------------------------------------------- */

/**
 * Splits a text line into a vector of tokens.
 *
 * @param line The string to split
 * @param tokens The vector of splitted tokens
 * @param sep The separator string used to recognize the tokens
 *
 * @return true if operation successfully completed, false otherwise
 */
bool splitLineInTokens(const std::string& line,
    std::vector<std::string>& tokens, const std::string& sep);


} // namespace Tools


/* -------------------------------------------------------------------------- */

#endif // __TOOLS_H__


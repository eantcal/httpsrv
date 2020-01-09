//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "FileUtils.h"
#include "StrUtils.h"

#include "picosha2.h"

#include <iomanip>
#include <sstream>
#include <random>

#ifdef WIN32
#include <direct.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#endif


/* -------------------------------------------------------------------------- */

bool FileUtils::createTemporaryDir(fs::path& path) {
   auto tmp_dir = fs::temp_directory_path();
   std::random_device dev;
   std::mt19937 prng(dev());
   std::uniform_int_distribution<uint64_t> rand(0);
   std::stringstream ss;
   ss << std::hex << rand(prng);
   path = tmp_dir / ss.str();
   return (fs::create_directory(path));
}


/* -------------------------------------------------------------------------- */

bool FileUtils::fileStat(
   const std::string& fileName,
   std::string& dateTime,
   std::string& ext,
   size_t& fsize)
{
   struct stat rstat = { 0 };
   int ret = stat(fileName.c_str(), &rstat);

   if (ret >= 0) {
      dateTime = ctime(&rstat.st_atime);
      fsize = rstat.st_size;

      std::string::size_type pos = fileName.rfind('.');
      ext = pos != std::string::npos
         ? fileName.substr(pos, fileName.size() - pos)
         : ".";

      StrUtils::removeLastCharIf(dateTime, '\n');

      return true;
   }

   return false;
}


/* -------------------------------------------------------------------------- */

std::string FileUtils::hashCode(const std::string& src)
{
   std::string id;
   picosha2::hash256_hex_string(src, id);
   return id;
}


/* -------------------------------------------------------------------------- */

bool FileUtils::touch(const std::string& fileName, bool createNewIfNotExists)
{
   std::fstream ofs;
   ofs.open(fileName, std::ofstream::out | std::ofstream::in);

   if (ofs) {
      ofs.seekg(0, ofs.end);
      auto length = ofs.tellg();
      ofs.seekg(0, ofs.beg);

      if (length > 0) {
         char c = 0;
         ofs.read(&c, 1);
         ofs.seekg(0, ofs.beg);
         ofs.write(&c, 1);
      }
      ofs.close();

      if (length < 1) {
         ofs.open(fileName, std::ofstream::out);
      }
   }
   else {
      if (createNewIfNotExists)
         ofs.open(fileName, std::ofstream::out);
   }

   return !ofs.fail();
}


/* -------------------------------------------------------------------------- */

std::string FileUtils::getHomeDir() {
   const char* homedir = nullptr;
#ifdef WIN32
   if ((homedir = getenv("USERPROFILE")) == nullptr) {
      homedir = ".";
   }
   return homedir;
#else
   if ((homedir = getenv("HOME")) == nullptr) {
      homedir = getpwuid(getuid())->pw_dir;
   }

   return homedir ? homedir : ".";
#endif
}


/* -------------------------------------------------------------------------- */

bool FileUtils::getFullPath(const std::string& partialPath, std::string& fullPath)
{
#ifdef WIN32
   char full[_MAX_PATH];

   if (_fullpath(full, partialPath.c_str(), _MAX_PATH) != 0) {
      fullPath = full;
      return true;
   }
#else
   auto actualpath = realpath(partialPath.c_str(), 0);
   if (actualpath) {
      fullPath = actualpath;
      free(actualpath);
      return true;
   }
#endif

   return false;
}


/* -------------------------------------------------------------------------- */

bool FileUtils::directoryExists(const std::string& pathName)
{
   fs::path dirPath(pathName);
   return fs::exists(dirPath) && fs::is_directory(dirPath);
}


/* -------------------------------------------------------------------------- */

bool FileUtils::touchDir(
   const std::string& relativeDirName,
   std::string& fullPath)
{
   // Create a new support directory if it does not exist
#ifdef WIN32
   _mkdir(relativeDirName.c_str());
#else
   mkdir(relativeDirName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

   return
      FileUtils::directoryExists(relativeDirName) &&
      FileUtils::getFullPath(relativeDirName, fullPath);
}

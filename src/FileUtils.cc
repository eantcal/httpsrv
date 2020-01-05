
#include "FileUtils.h"
#include "StrUtils.h"

#include "picosha2.h"

#include <iomanip>
#include <sstream>

#ifdef WIN32
#include <direct.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#endif

/* -------------------------------------------------------------------------- */

std::string FileUtils::initRepository(const std::string& path)
{
   std::string repositoryPath;

   // Resolve any homedir prefix
   std::string resPath;
   if (!path.empty()) {
      size_t prefixSize = path.size() > 1 ? 2 : 1;
      if ((prefixSize > 1 && path.substr(0, 2) == "~/") || path == "~") {
         resPath = getHomeDir();
         resPath += "/";
         resPath += path.substr(prefixSize, path.size() - prefixSize);
      }
   }

   if (!touchDir(resPath, repositoryPath)) {
      repositoryPath.clear();
   }

   return repositoryPath;
}


/* -------------------------------------------------------------------------- */

bool FileUtils::scanRepository(
   const std::string& path,
   TimeOrderedFileList& list,
   IdFileNameCache& idNameMap)
{
   fs::path dirPath(path);
   fs::directory_iterator endIt;

   idNameMap.clear(); // make sure the id/filename cache is empty

   if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
      list.clear();
      for (fs::directory_iterator it(dirPath); it != endIt; ++it) {
         if (fs::is_regular_file(it->status())) {
            list.insert({ fs::last_write_time(it->path()), *it });

            // update id/filename cache
            auto fName = it->path().filename().string();
            auto id = hashCode(fName);
            idNameMap.insert(id, fName);
         }
      }
      return true;
   }

   return false;
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

bool FileUtils::jsonStat(
   const std::string& filePath,  // actual file path (including name)
   const std::string& fileName,  // filename field of JSON output
   const std::string& id,        // id field of JSON output
   std::string& jsonOutput)
{
   struct stat rstat = { 0 };
   const int ret = stat(filePath.c_str(), &rstat);

   if (ret < 0)
      return false;

   std::tm bt = *localtime(&(rstat.st_atime));

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
   oss << "{" << std::endl;
   oss << "  \"id\": \"" << id << "\"," << std::endl;
   oss << "  \"name\": \"" << fileName << "\"," << std::endl;
   oss << "  \"size\": " << rstat.st_size << "," << std::endl;
   oss << "  \"timestamp\": \"" << ossTS.str() << "\"" << std::endl;
   oss << "}" << std::endl;
   jsonOutput = oss.str();

   return true;
}


/* -------------------------------------------------------------------------- */

bool FileUtils::touch(const std::string& fileName)
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

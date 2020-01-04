//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "Tools.h"
#include "picosha2.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif


#include <iostream>

/* -------------------------------------------------------------------------- */

void Tools::convertDurationInTimeval(const TimeoutInterval& d, timeval& tv)
{
    std::chrono::microseconds usec
        = std::chrono::duration_cast<std::chrono::microseconds>(d);

    if (usec <= std::chrono::microseconds(0)) {
        tv.tv_sec = tv.tv_usec = 0;
    } else {
        tv.tv_sec = static_cast<long>(usec.count() / 1000000LL);
        tv.tv_usec = static_cast<long>(usec.count() % 1000000LL);
    }
}


/* -------------------------------------------------------------------------- */

void Tools::getLocalTime(std::string& localTime)
{
    time_t ltime;
    ltime = ::time(NULL); // get current calendar time
    localTime = ::asctime(::localtime(&ltime));
    Tools::removeLastCharIf(localTime, '\n');
}


/* -------------------------------------------------------------------------- */

void Tools::removeLastCharIf(std::string& s, char c)
{
    while (!s.empty() && s.c_str()[s.size() - 1] == c)
        s = s.substr(0, s.size() - 1);
}


/* -------------------------------------------------------------------------- */

bool Tools::fileStat(
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

        Tools::removeLastCharIf(dateTime, '\n');

        return true;
    }

    return false;
}


/* -------------------------------------------------------------------------- */

std::string Tools::hashCode(const std::string& src)
{
    std::string id;
    picosha2::hash256_hex_string(src, id);
    return id;
}


/* -------------------------------------------------------------------------- */

bool Tools::jsonStat(
    const std::string filePath,  // actual file path (including name)
    const std::string& fileName, // filen name for JSON outout
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
    oss << "  \"id\": \"" << hashCode(fileName) << "\"," << std::endl;
    oss << "  \"name\": \"" << fileName << "\"," << std::endl;
    oss << "  \"size\": " << rstat.st_size << "," << std::endl;
    oss << "  \"timestamp\": \"" << ossTS.str() << "\"" << std::endl;
    oss << "}" << std::endl;
    jsonOutput = oss.str();

    return true;
}


/* -------------------------------------------------------------------------- */

bool Tools::touch(const std::string& fileName)
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

std::string Tools::getHomeDir() {
    const char *homedir = nullptr;
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

bool Tools::getFullPath(const std::string& partialPath, std::string& fullPath)
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

bool Tools::directoryExists(const std::string& pathName) 
{
    struct stat info;

    if (stat(pathName.c_str(), &info) != 0)
        return false;
    else if (S_ISDIR(info.st_mode))
        return true;

    return false;
}


/* -------------------------------------------------------------------------- */

bool Tools::touchDir(
    const std::string& relativeDirName,
    std::string& fullPath)
{
    // Create a new support directory if it does not exist
    mkdir(relativeDirName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    return
        Tools::directoryExists(relativeDirName) &&
        Tools::getFullPath(relativeDirName, fullPath);
}


/* -------------------------------------------------------------------------- */

bool Tools::splitLineInTokens(const std::string& line,
    std::vector<std::string>& tokens, const std::string& sep)
{
    if (line.empty() || line.size() < sep.size())
        return false;

    std::string subline = line;

    while (!subline.empty()) {
        size_t pos = subline.find(sep);

        if (pos == std::string::npos) {
            tokens.push_back(subline);
            return true;
        }

        tokens.push_back(subline.substr(0, pos));

        size_t off = pos + sep.size();

        subline = subline.substr(off, subline.size() - off);
    }

    return true;
}


/* -------------------------------------------------------------------------- */

std::string Tools::trim(const std::string& str)
{
    const auto strBegin = str.find_first_not_of(" \t\r\n");
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(" \t\r\n");
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

/* -------------------------------------------------------------------------- */


//
// This file is part of thttpd
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
#endif


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


/* ------------------------------------------------------------------------- */

std::string Tools::getTimestamp()
{
    using namespace std::chrono;

    // get current time
    auto now = system_clock::now();
    auto us = duration_cast<microseconds>(now.time_since_epoch());
    auto timer = system_clock::to_time_t(now);

    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;

    // YYYY-MM-DDTHH:MM:SS.uuuuuuZ
    oss << std::put_time(&bt, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(6) << us.count() % 1000000 << "Z";

    return oss.str();
}


/* -------------------------------------------------------------------------- */

bool Tools::jsonStat(const std::string& fileName, std::string& jsonOutput)
{
    struct stat rstat = { 0 };
    const int ret = stat(fileName.c_str(), &rstat);
    if (ret < 0)
        return false;

    std::tm bt = *localtime(&(rstat.st_atime));

    std::ostringstream ossTS;

#ifdef __linux__
    size_t microsec = (rstat.st_atim.tv_nsec / 1000) % 1000000;
#else
    size_t microsec = 0;
#endif
    // YYYY-MM-DDTHH:MM:SS.uuuuuuZ
    ossTS << std::put_time(&bt, "%Y-%m-%dT%H:%M:%S");
    ossTS << '.' << std::setfill('0') << std::setw(6) << microsec << "Z";

    std::string id;
    picosha2::hash256_hex_string(fileName, id);

    std::stringstream oss;
    oss << "\"id\"\": \"" << id << "\"" << std::endl;
    oss << "\"name\": \"" << fileName << "\"" << std::endl;
    oss << "\"size\": " << rstat.st_size << std::endl;
    oss << "\"timestamp\": \"" << ossTS.str() << "\"" << std::endl;

    return true;
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



//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __SYS_UTILS_H__
#define __SYS_UTILS_H__

/* -------------------------------------------------------------------------- */

#ifdef WIN32

/* -------------------------------------------------------------------------- */
// Windows

/* -------------------------------------------------------------------------- */

#include <WinSock2.h>
#include <direct.h>

#define stat _stat
typedef int socklen_t;

/* -------------------------------------------------------------------------- */

#else

/* -------------------------------------------------------------------------- */
// GNU C++

/* -------------------------------------------------------------------------- */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

/* -------------------------------------------------------------------------- */

typedef int errno_t;

/* -------------------------------------------------------------------------- */

#endif

/* -------------------------------------------------------------------------- */

#include <string>
#include <regex>
#include <chrono>
#include <ctime>

/* -------------------------------------------------------------------------- */

namespace SysUtils
{

using TimeoutInterval = std::chrono::system_clock::duration;

/**
 * Converts a timeval object into standard duration object
 *
 * @param d  Duration
 * @param tv Timeval source object
 */
void convertDurationInTimeval(const TimeoutInterval &d, timeval &tv);

/**
 * Initializes O/S Libraries. In case of failure
 * the function returns false
 *
 * @return true if operation is sucessfully completed,
 * false otherwise
 */
bool initCommunicationLib();

/**
 * Closes a socket descriptor.
 *
 * @return zero on success. On error, -1 is returned
 */
int closeSocketFd(int sd);

/**
 * Gets the system UTC time
 * Time format is "DoW Mon dd hh:mm:ss yyyy"
 * Example "Thu Sep 19 10:03:50 2013"
 *
 * @param retTime will contain the time
 */
void getUtcTime(std::string &retTime);

/**
 * Gets the system UTC time
 * Time format is "DoW Mon dd hh:mm:ss yyyy"
 * Example "Thu Sep 19 10:03:50 2013"
 *
 * @return the string will contain the time
 */
inline std::string getUtcTime()
{
   std::string lt;
   getUtcTime(lt);
   return lt;
}

} // namespace SysUtils

/* -------------------------------------------------------------------------- */

#endif // !__SYS_UTILS_H__

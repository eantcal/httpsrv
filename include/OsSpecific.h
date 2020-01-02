//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __OS_SPECIFIC__H__
#define __OS_SPECIFIC__H__


/* -------------------------------------------------------------------------- */

#include <string>
#include <sys/stat.h>
#include <sys/types.h>


/* -------------------------------------------------------------------------- */

namespace OsSpecific {


/* -------------------------------------------------------------------------- */

/**
 * Initializes O/S Libraries. In case of failure
 * the function returns false
 *
 * @return true if operation is sucessfully completed,
 * false otherwise
 */
bool initSocketLibrary();


/* -------------------------------------------------------------------------- */

/**
 * Closes a socket descriptor.
 *
 * @return zero on success. On error, -1 is returned
 */
int closeSocketFd(int sd);


/* -------------------------------------------------------------------------- */

}


/* -------------------------------------------------------------------------- */

#ifdef WIN32


/* -------------------------------------------------------------------------- */
// Windows

/* -------------------------------------------------------------------------- */

#include <WinSock2.h>
#include <direct.h>

#define stat _stat

#define mkdir(__dir, __ignored) _mkdir(__dir)
enum { S_IRWXU , S_IRWXG , S_IROTH , S_IXOTH };

inline bool S_ISDIR(int mask) noexcept { return (mask & S_IFDIR) == S_IFDIR; }

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


/* -------------------------------------------------------------------------- */

typedef int errno_t;


/* -------------------------------------------------------------------------- */

#endif


/* -------------------------------------------------------------------------- */

#endif // __OS_SPECIFIC__H__

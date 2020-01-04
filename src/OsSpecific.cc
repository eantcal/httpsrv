//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "OsSpecific.h"


/* -------------------------------------------------------------------------- */

#ifdef _MSC_VER


/* -------------------------------------------------------------------------- */
// MS Visual C++

/* -------------------------------------------------------------------------- */

#pragma comment(lib, "Ws2_32.lib")


/* -------------------------------------------------------------------------- */

bool OsSpecific::initSocketLibrary()
{
    // Socket library initialization
    WORD wVersionRequested = WINSOCK_VERSION;
    WSADATA wsaData = { 0 };

    return 0 == WSAStartup(wVersionRequested, &wsaData);
}


/* -------------------------------------------------------------------------- */

int OsSpecific::closeSocketFd(int sd) { 
    return ::closesocket(sd); 
}


/* -------------------------------------------------------------------------- */

#else

#include <signal.h>

/* -------------------------------------------------------------------------- */
// Other C++ platform


/* -------------------------------------------------------------------------- */

bool OsSpecific::initSocketLibrary() { 
    signal(SIGPIPE, SIG_IGN);
    return true; 
}


/* -------------------------------------------------------------------------- */

int OsSpecific::closeSocketFd(int sd) { 
    return ::close(sd); 
}


/* -------------------------------------------------------------------------- */

#endif

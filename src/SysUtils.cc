//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "SysUtils.h"
#include "StrUtils.h"


/* -------------------------------------------------------------------------- */

void SysUtils::convertDurationInTimeval(const TimeoutInterval& d, timeval& tv)
{
   std::chrono::microseconds usec
      = std::chrono::duration_cast<std::chrono::microseconds>(d);

   if (usec <= std::chrono::microseconds(0)) {
      tv.tv_sec = tv.tv_usec = 0;
   }
   else {
      tv.tv_sec = static_cast<long>(usec.count() / 1000000LL);
      tv.tv_usec = static_cast<long>(usec.count() % 1000000LL);
   }
}


/* -------------------------------------------------------------------------- */

void SysUtils::getLocalTime(std::string& localTime)
{
   time_t ltime;
   ltime = ::time(NULL); // get current calendar time
   localTime = ::asctime(::localtime(&ltime));
   StrUtils::removeLastCharIf(localTime, '\n');
}


/* -------------------------------------------------------------------------- */

#ifdef _MSC_VER


/* -------------------------------------------------------------------------- */
// MS Visual C++

/* -------------------------------------------------------------------------- */

#pragma comment(lib, "Ws2_32.lib")


/* -------------------------------------------------------------------------- */

bool SysUtils::initCommunicationLib()
{
   // Socket library initialization
   WORD wVersionRequested = WINSOCK_VERSION;
   WSADATA wsaData = { 0 };

   return 0 == WSAStartup(wVersionRequested, &wsaData);
}


/* -------------------------------------------------------------------------- */

int SysUtils::closeSocketFd(int sd) {
   return ::closesocket(sd);
}


/* -------------------------------------------------------------------------- */

#else

#include <signal.h>

/* -------------------------------------------------------------------------- */
// Other C++ platform


/* -------------------------------------------------------------------------- */

bool SysUtils::initCommunicationLib() {
   signal(SIGPIPE, SIG_IGN);
   return true;
}


/* -------------------------------------------------------------------------- */

int SysUtils::closeSocketFd(int sd) {
   return ::close(sd);
}


/* -------------------------------------------------------------------------- */

#endif

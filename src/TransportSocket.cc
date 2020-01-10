//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "TransportSocket.h"
#include "StrUtils.h"
#include "SysUtils.h"

#include <thread>

/* -------------------------------------------------------------------------- */

TransportSocket::~TransportSocket()
{
    if (isValid())
        SysUtils::closeSocketFd(getSocketFd());
}

/* -------------------------------------------------------------------------- */

TransportSocket::RecvEvent TransportSocket::waitForRecvEvent(
    const TransportSocket::TimeoutInterval &timeout)
{
    struct timeval tv_timeout = {0, 0};
    SysUtils::convertDurationInTimeval(timeout, tv_timeout);

    fd_set rd_mask;

    // Add socket to receive select mask
    FD_ZERO(&rd_mask);
    FD_SET(getSocketFd(), &rd_mask);

    long nd = select(FD_SETSIZE, &rd_mask, (fd_set *)0, (fd_set *)0, &tv_timeout);

    if (nd == 0)
        return RecvEvent::TIMEOUT;
    else if (nd < 0)
        return RecvEvent::RECV_ERROR;

    return RecvEvent::RECV_DATA;
}

/* -------------------------------------------------------------------------- */

int TransportSocket::sendFile(const std::string &filepath) noexcept
{
    std::ifstream ifs(filepath.c_str(), std::ios::in | std::ios::binary);

    if (!ifs.is_open())
        return -1;

    std::unique_ptr<char[]> buffer(new char[TX_BUFFER_SIZE]);

    int sent_bytes = 0;

    while (ifs.good())
    {
        ifs.read(buffer.get(), sizeof(buffer));

        int size = static_cast<int>(ifs.gcount());

        if (size == 0)
            return sent_bytes;

        if (size > 0)
        {
            int bsent = 0;

            // sent the whole buffer content
            while (bsent < size)
            {
                int txc = send(buffer.get(), size);
                if (txc < 0)
                    return -1;

                if (txc == 0)
                { // tx queue is congested ?
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                bsent += txc;
            }

            sent_bytes += bsent;
        }
        else
        {
            return -1;
        }
    }

    return sent_bytes;
}

/* -------------------------------------------------------------------------- */

bool TransportSocket::bind(const std::string& ip, const TranspPort& port)
{
   if (getSocketFd() <= 0)
      return false;

   sockaddr_in& sin = _local_ip_port_sa_in;

   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
   sin.sin_port = htons(port);

   return 0 ==
      ::bind(getSocketFd(), reinterpret_cast<const sockaddr*>(&sin), sizeof(sin));
}
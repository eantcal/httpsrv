//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "HttpSocket.h"
#include "Tools.h"

#include <vector>


/* -------------------------------------------------------------------------- */

HttpSocket& HttpSocket::operator=(TcpSocket::Handle handle)
{
    _socketHandle = handle;
    return *this;
}


/* -------------------------------------------------------------------------- */

bool HttpSocket::recv(HttpRequest::Handle& handle)
{
    char c = 0;
    int ret = 1;

    enum class CrLfSeq { CR1, LF1, CR2, LF2, IDLE } s = CrLfSeq::IDLE;

    auto crlf = [&s](char c) -> bool {
        switch (s) {
        case CrLfSeq::IDLE:
            s = (c == '\r') ? CrLfSeq::CR1 : CrLfSeq::IDLE;
            break;
        case CrLfSeq::CR1:
            s = (c == '\n') ? CrLfSeq::LF1 : CrLfSeq::IDLE;
            break;
        case CrLfSeq::LF1:
            s = (c == '\r') ? CrLfSeq::CR2 : CrLfSeq::IDLE;
            break;
        case CrLfSeq::CR2:
            s = (c == '\n') ? CrLfSeq::LF2 : CrLfSeq::IDLE;
            break;
        default:
            s = CrLfSeq::IDLE;
            break;
        }

        return s == CrLfSeq::LF2;
    };

    std::string line;
    std::string body;

    bool receivingBody = false;
    bool timeout = false;

    while (ret > 0 && _connUp && _socketHandle) {
        std::chrono::seconds sec(getConnectionTimeout());

        auto recvEv = _socketHandle->waitForRecvEvent(sec);

        switch (recvEv) {
        case TransportSocket::RecvEvent::RECV_ERROR:
            _connUp = false;
            break;
        case TransportSocket::RecvEvent::TIMEOUT:
            timeout = true;
            break;
        default:
            break;
        }

        if (!_connUp || timeout)
            break;

        ret = _socketHandle->recv(&c, 1);

        if (ret > 0) {
            if (receivingBody) 
                body += c;
            else
                line += c;
        } else if (ret <= 0) {
            _connUp = false;
            break;
        }

        if (receivingBody) {
            if (receivingBody && body.size() >= handle->getContentLength()) {
                _connUp = true;
                break;
            }
        } 
        else {
            if (crlf(c)) {
                if (!handle->isExpectedContinueResponse())
                    receivingBody = true;
                else
                    break;
            }

            if (s == CrLfSeq::LF1) {
                if (!line.empty()) {
                    handle->parseHeader(line);
                    line.clear();
                }
            }
        }
    }

    if (ret < 0 || !_socketHandle || handle->get_header().empty()) {
        return false;
    }

    std::string request = *handle->get_header().cbegin();
    std::vector<std::string> tokens;

    if (!Tools::splitLineInTokens(request, tokens, " ")) {
        return false;
    }

    if (tokens.size() != 3) {
        return false;
    }

    handle->parseMethod(tokens[0]);
    handle->parseUri(tokens[1]);
    handle->parseVersion(tokens[2]);

    std::cerr << "BODY:\n" << body << std::endl;

    if (!body.empty())
        handle->setBody(std::move(body));

    return true;
}


/* -------------------------------------------------------------------------- */

HttpSocket& HttpSocket::operator<<(const HttpResponse& response)
{
    const std::string& response_txt = response;
    size_t to_send = response_txt.size();

    while (to_send > 0) {
        int sent = _socketHandle->send(response);
        if (sent < 0) {
            _connUp = false;
            break;
        }
        to_send -= sent;
    }

    return *this;
}


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

HttpRequest::Handle HttpSocket::recv()
{
    HttpRequest::Handle handle(new (std::nothrow) HttpRequest);

    if (!handle)
        return nullptr;

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
            break;
        }

        return s == CrLfSeq::LF2;
    };

    std::string line;
    std::string body;

    int content_length = handle->getContentLength();
    bool receivingBody = false;

    while (ret > 0 && _connUp && _socketHandle) {
        std::chrono::seconds sec(getConnectionTimeout());

        auto recvEv = _socketHandle->waitForRecvEvent(sec);

        switch (recvEv) {
        case TransportSocket::RecvEvent::RECV_ERROR:
        case TransportSocket::RecvEvent::TIMEOUT:
            _connUp = false;
            break;
        default:
            break;
        }

        if (!_connUp)
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

        if (receivingBody && body.size() >= content_length) {
            _connUp = true;
            break;
        }

        if (crlf(c)) {
            receivingBody = true;
        }

        if (s == CrLfSeq::LF1 && !receivingBody) {
            if (!line.empty()) {
                handle->addHeader(line);
                line.clear();
            }
        }
    }

    if (ret < 0 || !_socketHandle || handle->get_header().empty()) {
        return handle;
    }

    std::string request = *handle->get_header().cbegin();
    std::vector<std::string> tokens;

    if (!Tools::splitLineInTokens(request, tokens, " ")) {
        return handle;
    }

    if (tokens.size() != 3) {
        return handle;
    }

    handle->parseMethod(tokens[0]);
    handle->parseUri(tokens[1]);
    handle->parseVersion(tokens[2]);

    std::cerr << "BODY:\n" << body << std::endl;

    if (!body.empty())
        handle->setBody(std::move(body));

    return handle;
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


//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

///\file config.h
///\brief HTTP Server configuration file


/* -------------------------------------------------------------------------- */

#ifndef __HTTP_CONFIG_H__
#define __HTTP_CONFIG_H__

#ifdef WIN32
#define HTTP_SERVER_WROOT "~/httpsrv"
#else
#define HTTP_SERVER_WROOT "~/.httpsrv"
#endif
#define HTTP_SERVER_PORT 8080
#define HTTP_SERVER_NAME "httpsrv"
#define HTTP_SERVER_MAJ_V 1
#define HTTP_SERVER_MIN_V 0
#define HTTP_SERVER_TX_BUF_SIZE 0x100000
#define HTTP_SERVER_BACKLOG SOMAXCONN
#define HTTP_CONNECTION_TIMEOUT_MS 10
#define HTTP_SERVER_VER "HTTP/1.1"

#define HTTP_SERVER_POST_STORE       "/store"
#define HTTP_SERVER_GET_FILES        "/files"
#define HTTP_SERVER_GET_MRUFILES     "/mrufiles"
#define HTTP_SERVER_GET_MRUFILES_ZIP "/mrufiles/zip"


#endif // __HTTP_CONFIG_H__


/* -------------------------------------------------------------------------- */


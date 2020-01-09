//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

///\file config.h
///\brief configuration file

/* -------------------------------------------------------------------------- */

#ifndef __HTTP_CONFIG_H__
#define __HTTP_CONFIG_H__

#ifdef WIN32
#define HTTP_SERVER_LOCAL_REPOSITORY_PATH "~/httpsrv"
#else
#define HTTP_SERVER_LOCAL_REPOSITORY_PATH "~/.httpsrv"
#endif
#define HTTP_SERVER_PORT 8080
#define HTTP_SERVER_NAME "httpsrv"
#define HTTP_SERVER_MAJ_V 1
#define HTTP_SERVER_MIN_V 0
#define HTTP_SERVER_TX_BUF_SIZE 0x100000
#define HTTP_SERVER_BACKLOG SOMAXCONN
#define HTTP_CONNECTION_TIMEOUT_MS 10
#define HTTP_SERVER_VER "HTTP/1.1"

#define HTTP_URIPFX_FILES "files"
#define HTTP_URISFX_ZIP "zip"
#define MRU_FILES_ZIP_NAME "mrufiles.zip"

#define HTTP_SERVER_POST_STORE "/store"
#define HTTP_SERVER_GET_FILES "/" HTTP_URIPFX_FILES
#define HTTP_SERVER_GET_MRUFILES "/mrufiles"
#define HTTP_SERVER_GET_MRUFILES_ZIP "/mrufiles/" HTTP_URISFX_ZIP

#define MRUFILES_DEF_N 3
#define MRUFILES_MAX_N 1000

/* -------------------------------------------------------------------------- */

#endif // __HTTP_CONFIG_H__

/* -------------------------------------------------------------------------- */

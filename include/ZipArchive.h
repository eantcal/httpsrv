//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __ZIP_ARCHIVE_H__
#define __ZIP_ARCHIVE_H__

/* -------------------------------------------------------------------------- */

#include "zip.h"
#include <string>

#include "FileUtils.h"


/* -------------------------------------------------------------------------- */

class ZipArchive
{
public:
   //! ctor
   ZipArchive(const std::string &fileName) : _fileName(fileName)
   {
   }

   //! Creates a new zip archive
   bool create()
   {
      _zipHandler =
          zip_open(_fileName.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
      return _zipHandler != nullptr;
   }

   //! Add a new file (fileName) as zipEntryName into the zip archive
   bool add(const std::string &fileName, const std::string &zipEntryName)
   {
      if (0 == zip_entry_open(_zipHandler, zipEntryName.c_str()))
      {
         if (0 == zip_entry_fwrite(_zipHandler, fileName.c_str()))
            return 0 == zip_entry_close(_zipHandler);

         zip_entry_close(_zipHandler);
      }
      return false;
   }

   //! Flush and close zip archive
   void close()
   {
      if (!_zipHandler)
         return;

      zip_close(_zipHandler);
      _zipHandler = nullptr;
   }

   //! dtor (calls close())
   ~ZipArchive()
   {
      close();
   }

private:
   std::string _fileName;
   struct zip_t *_zipHandler = nullptr;
};

/* -------------------------------------------------------------------------- */

#endif // ! __ZIP_ARCHIVE_H__

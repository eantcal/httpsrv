//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "Application.h"


/* -------------------------------------------------------------------------- */

/**
 * Program entry point
 */
int main(int argc, char* argv[])
{
   Application application(argc, argv, std::cout);
   auto errCode = application.run();

   switch (errCode) {
      case Application::ErrCode::success:
      default:
         break;

      case Application::ErrCode::showVersionUsage:
         std::cout << application.getError() << std::endl;
         return 0;

      case Application::ErrCode::commandLineError:
      case Application::ErrCode::fileRepositoryInitError:
      case Application::ErrCode::idFileNameCacheInitError:
      case Application::ErrCode::httpSrvBindError:
      case Application::ErrCode::httpSrvListenError:
      case Application::ErrCode::httpSrvStartError:
         std::cerr << application.getError() << std::endl;
         return 1;
   }

   return 0;
}

//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#ifndef __STR_UTILS_H__
#define __STR_UTILS_H__


/* -------------------------------------------------------------------------- */

#include <string>
#include <vector>
#include <algorithm>


/* -------------------------------------------------------------------------- */

namespace StrUtils {

   /**
    *  Removes any instances of character c if present at the end of the 
    *  string s
    *
    *  @param s The input / ouput string
    *  @param c Searching character to remove
    */
   void removeLastCharIf(std::string& s, char c);


   /**
    * Splits a text line into a vector of tokens.
    *
    * @param line The string to split
    * @param tokens The vector of splitted tokens
    * @param sep The separator string used to recognize the tokens
    *
    * @return true if operation successfully completed, false otherwise
    */
   bool splitLineInTokens(const std::string& line,
         std::vector<std::string>& tokens, const std::string& sep);


   /**
    * Eliminates leading and trailing spaces a give string
    *
    * @param str string to trim
    * @return new trimmed string
    */
   std::string trim(const std::string& str);


   /**
    * Convert a given string to uppercase
    *
    * @param str string to convert
    * @return new uppercased string
    */
   inline static std::string uppercase(const std::string& str) {
      std::string ret = str;
      std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
      return ret;
   }


} // namespace StrUtils


/* -------------------------------------------------------------------------- */

#endif // __STR_UTILS_H__

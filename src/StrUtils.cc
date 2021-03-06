//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "StrUtils.h"

#include <sstream>
#include <iomanip>

/* -------------------------------------------------------------------------- */

void StrUtils::removeLastCharIf(std::string &s, char c)
{
   while (!s.empty() && s.c_str()[s.size() - 1] == c)
      s = s.substr(0, s.size() - 1);
}

/* -------------------------------------------------------------------------- */

bool StrUtils::splitLineInTokens(
    const std::string &line,
    std::vector<std::string> &tokens, const std::string &sep)
{
   if (line.empty() || line.size() < sep.size())
      return false;

   std::string subline = line;

   while (!subline.empty())
   {
      size_t pos = subline.find(sep);

      if (pos == std::string::npos)
      {
         tokens.push_back(subline);
         return true;
      }

      tokens.push_back(subline.substr(0, pos));

      size_t off = pos + sep.size();

      subline = subline.substr(off, subline.size() - off);
   }

   return true;
}

/* -------------------------------------------------------------------------- */

std::string StrUtils::trim(const std::string &str)
{
   const auto strBegin = str.find_first_not_of(" \t\r\n");
   if (strBegin == std::string::npos)
      return ""; // no content

   const auto strEnd = str.find_last_not_of(" \t\r\n");
   const auto strRange = strEnd - strBegin + 1;

   return str.substr(strBegin, strRange);
}

/* -------------------------------------------------------------------------- */

std::string StrUtils::escapeJson(const std::string& str) 
{
    std::ostringstream ss;

    for (const auto & ch : str) 
    {
        switch (ch) {
           case '\\': ss << "\\\\"; break;
           case '"': ss << "\\\""; break;
           case '\t': ss << "\\t"; break;
           case '\r': ss << "\\r"; break;
           case '\b': ss << "\\b"; break;
           case '\n': ss << "\\n"; break;
           case '\f': ss << "\\f"; break;
           default:
               if ('\x00' <= ch && ch <= '\x1f') 
               {
                   ss << "\\u"
                     << std::hex << std::setw(4) 
                     << std::setfill('0') << int(ch);
               } 
               else 
               {
                   ss << ch;
               }
        }
    }
    return ss.str();
}
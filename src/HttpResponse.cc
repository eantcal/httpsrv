//
// This file is part of httpsrv
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "HttpResponse.h"
#include "Tools.h"
#include "config.h"

/* -------------------------------------------------------------------------- */

void HttpResponse::formatError(
   std::string& output, int code, const std::string& msg)
{
   std::string scode = std::to_string(code);

   std::string error_html = "<html><head><title>" + scode + " " + msg
      + "</title></head>" + "<body>Sorry, I can't do that</body></html>\r\n";

   output = HTTP_SERVER_VER " " + scode + " " + msg + "\r\n";
   output += "Date: " + Tools::getLocalTime() + "\r\n";
   output += "Server: " HTTP_SERVER_NAME "\r\n";
   output += "Content-Length: " + std::to_string(error_html.size()) + "\r\n";
   output += "Content-Type: text/html\r\n\r\n";
   output += error_html;
}


/* -------------------------------------------------------------------------- */

void HttpResponse::formatPositiveResponse(
   std::string& response,
   const std::string& fileTime,
   const std::string& fileExt,
   const size_t& contentLen)
{

   response = HTTP_SERVER_VER " 200 OK\r\n";
   response += "Date: " + Tools::getLocalTime() + "\r\n";
   response += "Server: " HTTP_SERVER_NAME "\r\n";
   response += "Content-Length: " + std::to_string(contentLen) + "\r\n";
   // response += "Connection: Keep-Alive\r\n";
   response += "Last Modified: " + fileTime + "\r\n";
   response += "Content-Type: ";

   // Resolve mime type using the uri/file extension
   auto it = _mimeTbl.find(fileExt);

   response
      += it != _mimeTbl.end() ? it->second : "application/octet-stream";

   // Close the rensponse header by using the sequence CRFL twice
   response += "\r\n\r\n";
}


/* -------------------------------------------------------------------------- */

void HttpResponse::formatContinueResponse(std::string& response)
{
   response = HTTP_SERVER_VER " 100 Continue\r\n\r\n";
}


/* -------------------------------------------------------------------------- */

HttpResponse::HttpResponse(
   const HttpRequest& request,
   const std::string& webRootPath,
   const std::string& body,
   const std::string& bodyFormat)
{
   if (request.getMethod() == HttpRequest::Method::UNKNOWN) {
      formatError(_response, 403, "Forbidden");
      return;
   }

   auto rpath = [](std::string& s) {
      if (!s.empty() && s[0] != '/')
         s = "/" + s;
   };

   _localRepositoryPath = webRootPath;

   _localUriPath = request.getUri();
   rpath(_localUriPath);
   _localUriPath = webRootPath + _localUriPath;

   std::string fileTime, fileExt;
   size_t contentLen = 0;

   if (request.getMethod() == HttpRequest::Method::POST) {
      if (request.isExpectedContinueResponse()) {
         formatContinueResponse(_response);
      }
      else {
         if (request.getUri() != HTTP_SERVER_POST_STORE) {
            formatError(_response, 400, "Bad Request Error");
         }
         else if (body.empty()) {
            formatError(_response, 500, "Internal Server Error");
         }
         else {
            formatPositiveResponse(
               _response,
               Tools::getLocalTime(),
               std::string(bodyFormat),
               body.size());

            _response += body;
         }
      }
   }
   else { // GET/HEAD
      if (Tools::fileStat(_localUriPath, fileTime, fileExt, contentLen)) {
         formatPositiveResponse(_response, fileTime, fileExt, contentLen);
      }
      else {
         formatError(_response, 404, "Not Found");
      }
   }
}


/* -------------------------------------------------------------------------- */

std::ostream& HttpResponse::dump(std::ostream& os, const std::string& id)
{
   std::string ss;
   ss = "<<< RESPONSE " + id + "\n";
   ss += _response;
   os << ss << "\n";

   return os;
}


/* -------------------------------------------------------------------------- */

std::unordered_map<std::string, std::string> HttpResponse::_mimeTbl = {
    { ".3dm", "x-world/x-3dmf" }, { ".3dmf", "x-world/x-3dmf" },
    { ".a", "application/octet-stream" },
    { ".aab", "application/x-authorware-bin" },
    { ".aam", "application/x-authorware-map" },
    { ".aas", "application/x-authorware-seg" }, { ".abc", "text/vnd.abc" },
    { ".acgi", "text/html" }, { ".afl", "video/animaflex" },
    { ".ai", "application/postscript" }, { ".aif", "audio/aiff" },
    { ".aifc", "audio/aiff" }, { ".aiff", "audio/aiff" },
    { ".aim", "application/x-aim" }, { ".aip", "text/x-audiosoft-intra" },
    { ".ani", "application/x-navi-animation" },
    { ".aos", "application/x-nokia-9000-communicator-add-on-software" },
    { ".aps", "application/mime" }, { ".arc", "application/octet-stream" },
    { ".arj", "application/arj" }, { ".art", "image/x-jg" },
    { ".asf", "video/x-ms-asf" }, { ".asm", "text/x-asm" },
    { ".asp", "text/asp" }, { ".asx", "application/x-mplayer2" },
    { ".au", "audio/basic" }, { ".avi", "video/avi" },
    { ".avs", "video/avs-video" }, { ".bcpio", "application/x-bcpio" },
    { ".bin", "application/octet-stream" }, { ".bm", "image/bmp" },
    { ".bmp", "image/bmp" }, { ".boo", "application/book" },
    { ".book", "application/book" }, { ".boz", "application/x-bzip2" },
    { ".bsh", "application/x-bsh" }, { ".bz", "application/x-bzip" },
    { ".bz2", "application/x-bzip2" }, { ".c", "text/plain" },
    { ".c++", "text/plain" }, { ".cat", "application/vnd.ms-pki.seccat" },
    { ".cc", "text/x-c" }, { ".ccad", "application/clariscad" },
    { ".cco", "application/x-cocoa" }, { ".cdf", "application/cdf" },
    { ".cer", "application/pkix-cert" }, { ".cha", "application/x-chat" },
    { ".chat", "application/x-chat" }, { ".class", "application/java" },
    { ".com", "application/octet-stream" }, { ".com", "text/plain" },
    { ".conf", "text/plain" }, { ".cpio", "application/x-cpio" },
    { ".cpp", "text/x-c" }, { ".cpt", "application/x-cpt" },
    { ".crl", "application/pkcs-crl" }, { ".crt", "application/pkix-cert" },
    { ".csh", "application/x-csh" }, { ".csh", "text/x-script.csh" },
    { ".css", "text/css" }, { ".cxx", "text/plain" },
    { ".dcr", "application/x-director" }, { ".deepv", "application/x-deepv" },
    { ".def", "text/plain" }, { ".der", "application/x-x509-ca-cert" },
    { ".dif", "video/x-dv" }, { ".dir", "application/x-director" },
    { ".dl", "video/dl" }, { ".doc", "application/msword" },
    { ".docx", "application/"
               "vnd.openxmlformats-officedocument.wordprocessingml.document" },
    { ".dot", "application/msword" }, { ".dp", "application/commonground" },
    { ".drw", "application/drafting" }, { ".dump", "application/octet-stream" },
    { ".dv", "video/x-dv" }, { ".dvi", "application/x-dvi" },
    { ".dwf", "drawing/x-dwf (old)" }, { ".dwf", "model/vnd.dwf" },
    { ".dwg", "application/acad" }, { ".dxf", "application/dxf" },
    { ".dxr", "application/x-director" }, { ".el", "text/x-script.elisp" },
    { ".elc", "application/x-elc" }, { ".env", "application/x-envoy" },
    { ".eps", "application/postscript" }, { ".es", "application/x-esrehber" },
    { ".etx", "text/x-setext" }, { ".evy", "application/envoy" },
    { ".exe", "application/octet-stream" }, { ".f", "text/x-fortran" },
    { ".f77", "text/x-fortran" }, { ".f90", "text/x-fortran" },
    { ".fdf", "application/vnd.fdf" }, { ".fif", "image/fif" },
    { ".fli", "video/fli" }, { ".flo", "image/florian" },
    { ".flx", "text/vnd.fmi.flexstor" }, { ".fmf", "video/x-atomic3d-feature" },
    { ".for", "text/x-fortran" }, { ".fpx", "image/vnd.fpx" },
    { ".frl", "application/freeloader" }, { ".funk", "audio/make" },
    { ".g", "text/plain" }, { ".g3", "image/g3fax" }, { ".gif", "image/gif" },
    { ".gl", "video/gl" }, { ".gsd", "audio/x-gsm" }, { ".gsm", "audio/x-gsm" },
    { ".gsp", "application/x-gsp" }, { ".gss", "application/x-gss" },
    { ".gtar", "application/x-gtar" }, { ".gz", "application/x-compressed" },
    { ".gz", "application/x-gzip" }, { ".gzip", "application/x-gzip" },
    { ".h", "text/x-h" }, { ".hdf", "application/x-hdf" },
    { ".help", "application/x-helpfile" },
    { ".hgl", "application/vnd.hp-hpgl" }, { ".hh", "text/x-h" },
    { ".hlb", "text/x-script" }, { ".hlp", "application/hlp" },
    { ".hpg", "application/vnd.hp-hpgl" },
    { ".hpgl", "application/vnd.hp-hpgl" }, { ".hqx", "application/binhex" },
    { ".hqx", "application/binhex4" }, { ".hta", "application/hta" },
    { ".htc", "text/x-component" }, { ".htm", "text/html" },
    { ".html", "text/html" }, { ".htmls", "text/html" },
    { ".htt", "text/webviewhtml" }, { ".htx", "text/html" },
    { ".ice", "x-conference/x-cooltalk" }, { ".ico", "image/x-icon" },
    { ".idc", "text/plain" }, { ".ief", "image/ief" }, { ".iefs", "image/ief" },
    { ".iges", "application/iges" }, { ".iges", "model/iges" },
    { ".igs", "application/iges" }, { ".igs", "model/iges" },
    { ".ima", "application/x-ima" }, { ".imap", "application/x-httpd-imap" },
    { ".inf", "application/inf" }, { ".ins", "application/x-internett-signup" },
    { ".ip", "application/x-ip2" }, { ".isu", "video/x-isvideo" },
    { ".it", "audio/it" }, { ".iv", "application/x-inventor" },
    { ".ivr", "i-world/i-vrml" }, { ".ivy", "application/x-livescreen" },
    { ".jam", "audio/x-jam" }, { ".jav", "text/x-java-source" },
    { ".java", "text/x-java-source" },
    { ".jcm", "application/x-java-commerce" }, { ".jfif", "image/jpeg" },
    { ".jfif", "image/pjpeg" }, { ".jpe", "image/jpeg" },
    { ".jpeg", "image/jpeg" }, { ".jpg", "image/jpeg" },
    { ".jps", "image/x-jps" }, { ".js", "application/javascript" },
    { ".jut", "image/jutvision" }, { ".kar", "audio/midi" },
    { ".ksh", "application/x-ksh" }, { ".la", "audio/nspaudio" },
    { ".lam", "audio/x-liveaudio" }, { ".latex", "application/x-latex" },
    { ".lha", "application/lha" }, { ".lhx", "application/octet-stream" },
    { ".list", "text/plain" }, { ".lma", "audio/nspaudio" },
    { ".lma", "audio/x-nspaudio" }, { ".log", "text/plain" },
    { ".lsp", "application/x-lisp" }, { ".lst", "text/plain" },
    { ".lsx", "text/x-la-asf" }, { ".ltx", "application/x-latex" },
    { ".lzh", "application/x-lzh" }, { ".lzx", "application/lzx" },
    { ".m", "text/x-m" }, { ".m1v", "video/mpeg" }, { ".m2a", "audio/mpeg" },
    { ".m2v", "video/mpeg" }, { ".m3u", "audio/x-mpequrl" },
    { ".man", "application/x-troff-man" }, { ".map", "application/x-navimap" },
    { ".mar", "text/plain" }, { ".mbd", "application/mbedlet" },
    { ".mcd", "application/mcad" }, { ".mcf", "image/vasa" },
    { ".mcf", "text/mcf" }, { ".mcp", "application/netmc" },
    { ".me", "application/x-troff-me" }, { ".mht", "message/rfc822" },
    { ".mhtml", "message/rfc822" }, { ".mid", "audio/midi" },
    { ".midi", "audio/midi" }, { ".mif", "application/x-frame" },
    { ".mime", "message/rfc822" },
    { ".mjf", "audio/x-vnd.audioexplosion.mjuicemediafile" },
    { ".mjpg", "video/x-motion-jpeg" }, { ".mm", "application/base64" },
    { ".mme", "application/base64" }, { ".mod", "audio/mod" },
    { ".moov", "video/quicktime" }, { ".mov", "video/quicktime" },
    { ".movie", "video/x-sgi-movie" }, { ".mp2", "audio/mpeg" },
    { ".mp3", "audio/mpeg3" }, { ".mpa", "audio/mpeg" },
    { ".mpc", "application/x-project" }, { ".mpe", "video/mpeg" },
    { ".mpeg", "video/mpeg" }, { ".mpg", "audio/mpeg" },
    { ".mpga", "audio/mpeg" }, { ".mpp", "application/vnd.ms-project" },
    { ".mpt", "application/x-project" }, { ".mpv", "application/x-project" },
    { ".mpx", "application/x-project" }, { ".mrc", "application/marc" },
    { ".ms", "application/x-troff-ms" }, { ".mv", "video/x-sgi-movie" },
    { ".my", "audio/make" }, { ".mzz", "application/x-vnd.audioexplosion.mzz" },
    { ".nap", "image/naplps" }, { ".naplps", "image/naplps" },
    { ".nc", "application/x-netcdf" },
    { ".ncm", "application/vnd.nokia.configuration-message" },
    { ".nif", "image/x-niff" }, { ".niff", "image/x-niff" },
    { ".nix", "application/x-mix-transfer" },
    { ".nsc", "application/x-conference" }, { ".nvd", "application/x-navidoc" },
    { ".o", "application/octet-stream" }, { ".oda", "application/oda" },
    { ".omc", "application/x-omc" }, { ".omcd", "application/x-omcdatamaker" },
    { ".omcr", "application/x-omcregerator" }, { ".p", "text/x-pascal" },
    { ".p10", "application/pkcs10" }, { ".p12", "application/pkcs-12" },
    { ".p7a", "application/x-pkcs7-signature" },
    { ".p7c", "application/pkcs7-mime" },
    { ".p7c", "application/x-pkcs7-mime" },
    { ".p7m", "application/pkcs7-mime" },
    { ".p7r", "application/x-pkcs7-certreqresp" },
    { ".p7s", "application/pkcs7-signature" },
    { ".part", "application/pro_eng" }, { ".pas", "text/pascal" },
    { ".pbm", "image/x-portable-bitmap" }, { ".pcl", "application/vnd.hp-pcl" },
    { ".pct", "image/x-pict" }, { ".pcx", "image/x-pcx" },
    { ".pdb", "chemical/x-pdb" }, { ".pdf", "application/pdf" },
    { ".pfunk", "audio/make" }, { ".pgm", "image/x-portable-graymap" },
    { ".pic", "image/pict" }, { ".pict", "image/pict" },
    { ".pkg", "application/x-newton-compatible-pkg" },
    { ".pko", "application/vnd.ms-pki.pko" }, { ".pl", "text/x-script.perl" },
    { ".plx", "application/x-pixclscript" },
    { ".pm", "text/x-script.perl-module" },
    { ".pm4", "application/x-pagemaker" },
    { ".pm5", "application/x-pagemaker" }, { ".png", "image/png" },
    { ".pnm", "application/x-portable-anymap" },
    { ".pnm", "image/x-portable-anymap" },
    { ".pot", "application/vnd.ms-powerpoint" },
    { ".pot", "application/vnd.ms-powerpoint" }, { ".pov", "model/x-pov" },
    { ".ppa", "application/vnd.ms-powerpoint" },
    { ".ppm", "image/x-portable-pixmap" },
    { ".pps", "application/vnd.ms-powerpoint" },
    { ".ppt", "application/vnd.ms-powerpoint" },
    { ".ppz", "application/vnd.ms-powerpoint" },
    { ".pre", "application/x-freelance" }, { ".prt", "application/pro_eng" },
    { ".ps", "application/postscript" }, { ".psd", "application/octet-stream" },
    { ".pvu", "paleovu/x-pv" }, { ".pwz", "application/vnd.ms-powerpoint" },
    { ".py", "text/x-script.phyton" },
    { ".pyc", "applicaiton/x-bytecode.python" }, { ".qcp", "audio/vnd.qcelp" },
    { ".qd3", "x-world/x-3dmf" }, { ".qd3d", "x-world/x-3dmf" },
    { ".qif", "image/x-quicktime" }, { ".qt", "video/quicktime" },
    { ".qtc", "video/x-qtc" }, { ".qti", "image/x-quicktime" },
    { ".qtif", "image/x-quicktime" }, { ".ra", "audio/x-pn-realaudio" },
    { ".ram", "audio/x-pn-realaudio" }, { ".ras", "image/cmu-raster" },
    { ".rast", "image/cmu-raster" }, { ".rexx", "text/x-script.rexx" },
    { ".rf", "image/vnd.rn-realflash" }, { ".rgb", "image/x-rgb" },
    { ".rm", "audio/x-pn-realaudio" }, { ".rmi", "audio/mid" },
    { ".rmm", "audio/x-pn-realaudio" }, { ".rmp", "audio/x-pn-realaudio" },
    { ".rng", "application/ringing-tones" },
    { ".rnx", "application/vnd.rn-realplayer" },
    { ".roff", "application/x-troff" }, { ".rp", "image/vnd.rn-realpix" },
    { ".rpm", "audio/x-pn-realaudio-plugin" }, { ".rt", "text/richtext" },
    { ".rtf", "application/rtf" }, { ".rtx", "application/rtf" },
    { ".rtx", "text/richtext" }, { ".rv", "video/vnd.rn-realvideo" },
    { ".s", "text/x-asm" }, { ".s3m", "audio/s3m" },
    { ".saveme", "application/octet-stream" },
    { ".sbk", "application/x-tbook" },
    { ".scm", "application/x-lotusscreencam" },
    { ".scm", "text/x-script.guile" }, { ".sdml", "text/plain" },
    { ".sdp", "application/sdp" }, { ".sdr", "application/sounder" },
    { ".sea", "application/sea" }, { ".set", "application/set" },
    { ".sgm", "text/sgml" }, { ".sgml", "text/sgml" },
    { ".sh", "application/x-sh" }, { ".shar", "application/x-shar" },
    { ".shtml", "text/html" }, { ".sid", "audio/x-psid" },
    { ".sit", "application/x-sit" }, { ".skd", "application/x-koan" },
    { ".skm", "application/x-koan" }, { ".skp", "application/x-koan" },
    { ".skt", "application/x-koan" }, { ".sl", "application/x-seelogo" },
    { ".smi", "application/smil" }, { ".smil", "application/smil" },
    { ".snd", "audio/basic" }, { ".snd", "audio/x-adpcm" },
    { ".sol", "application/solids" }, { ".spc", "text/x-speech" },
    { ".spl", "application/futuresplash" }, { ".spr", "application/x-sprite" },
    { ".sprite", "application/x-sprite" },
    { ".src", "application/x-wais-source" },
    { ".ssi", "text/x-server-parsed-html" },
    { ".ssm", "application/streamingmedia" },
    { ".sst", "application/vnd.ms-pki.certstore" },
    { ".step", "application/step" }, { ".stl", "application/sla" },
    { ".stp", "application/step" }, { ".sv4cpio", "application/x-sv4cpio" },
    { ".sv4crc", "application/x-sv4crc" }, { ".svf", "image/vnd.dwg" },
    { ".svr", "application/x-world" },
    { ".swf", "application/x-shockwave-flash" },
    { ".t", "application/x-troff" }, { ".talk", "text/x-speech" },
    { ".tar", "application/x-tar" }, { ".tbk", "application/toolbook" },
    { ".tcl", "application/x-tcl" }, { ".tcsh", "text/x-script.tcsh" },
    { ".tex", "application/x-tex" }, { ".texi", "application/x-texinfo" },
    { ".texinfo", "application/x-texinfo" }, { ".text", "text/plain" },
    { ".tgz", "application/x-compressed" }, { ".tif", "image/tiff" },
    { ".tiff", "image/x-tiff" }, { ".tr", "application/x-troff" },
    { ".tsi", "audio/tsp-audio" }, { ".tsp", "audio/tsplayer" },
    { ".tsv", "text/tab-separated-values" }, { ".turbot", "image/florian" },
    { ".txt", "text/plain" }, { ".uil", "text/x-uil" },
    { ".uni", "text/uri-list" }, { ".unis", "text/uri-list" },
    { ".unv", "application/i-deas" }, { ".uri", "text/uri-list" },
    { ".uris", "text/uri-list" }, { ".ustar", "application/x-ustar" },
    { ".uu", "text/x-uuencode" }, { ".uue", "text/x-uuencode" },
    { ".vcd", "application/x-cdlink" }, { ".vcs", "text/x-vcalendar" },
    { ".vda", "application/vda" }, { ".vdo", "video/vdo" },
    { ".vew", "application/groupwise" }, { ".viv", "video/vivo" },
    { ".viv", "video/vnd.vivo" }, { ".vivo", "video/vivo" },
    { ".vmd", "application/vocaltec-media-desc" },
    { ".vmf", "application/vocaltec-media-file" }, { ".voc", "audio/voc" },
    { ".vos", "video/vosaic" }, { ".vox", "audio/voxware" },
    { ".vqe", "audio/x-twinvq-plugin" }, { ".vqf", "audio/x-twinvq" },
    { ".vql", "audio/x-twinvq-plugin" }, { ".vrml", "model/vrml" },
    { ".vrt", "x-world/x-vrt" }, { ".vsd", "application/x-visio" },
    { ".vst", "application/x-visio" }, { ".vsw", "application/x-visio" },
    { ".w60", "application/wordperfect6.0" },
    { ".w61", "application/wordperfect6.1" }, { ".w6w", "application/msword" },
    { ".wav", "audio/wav" }, { ".wb1", "application/x-qpro" },
    { ".wbmp", "image/vnd.wap.wbmp" }, { ".web", "application/vnd.xara" },
    { ".wiz", "application/msword" }, { ".wk1", "application/x-123" },
    { ".wmf", "windows/metafile" }, { ".wml", "text/vnd.wap.wml" },
    { ".wmlc", "application/vnd.wap.wmlc" },
    { ".wmls", "text/vnd.wap.wmlscript" },
    { ".wmlsc", "application/vnd.wap.wmlscriptc" },
    { ".word", "application/msword" }, { ".wp", "application/wordperfect" },
    { ".wp5", "application/wordperfect" },
    { ".wp5", "application/wordperfect6.0" },
    { ".wp6", "application/wordperfect" },
    { ".wpd", "application/wordperfect" }, { ".wpd", "application/x-wpwin" },
    { ".wq1", "application/x-lotus" }, { ".wri", "application/mswrite" },
    { ".wrl", "model/vrml" }, { ".wrz", "model/vrml" },
    { ".wsc", "text/scriplet" }, { ".wsrc", "application/x-wais-source" },
    { ".wtk", "application/x-wintalk" }, { ".xbm", "image/x-xbitmap" },
    { ".xdr", "video/x-amt-demorun" }, { ".xgz", "xgl/drawing" },
    { ".xif", "image/vnd.xiff" }, { ".xl", "application/vnd.ms-excel" },
    { ".xla", "application/vnd.ms-excel" },
    { ".xlb", "application/vnd.ms-excel" },
    { ".xlc", "application/vnd.ms-excel" },
    { ".xld", "application/vnd.ms-excel" },
    { ".xlk", "application/vnd.ms-excel" },
    { ".xll", "application/vnd.ms-excel" },
    { ".xlm", "application/vnd.ms-excel" },
    { ".xls", "application/vnd.ms-excel" },
    { ".xlt", "application/vnd.ms-excel" },
    { ".xlv", "application/vnd.ms-excel" },
    { ".xlw", "application/vnd.ms-excel" }, { ".xm", "audio/xm" },
    { ".xml", "application/xml" }, { ".xmz", "xgl/movie" },
    { ".xpix", "application/x-vnd.ls-xpix" }, { ".xpm", "image/xpm" },
    { ".xsr", "video/x-amt-showrun" }, { ".xwd", "image/x-xwd" },
    { ".xyz", "chemical/x-pdb" }, { ".z", "application/x-compress" },
    { ".zip", "application/zip" }, { ".zoo", "application/octet-stream" },
    { ".zsh", "text/x-script.zsh" },
};


/* -------------------------------------------------------------------------- */

/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Utils.h"

#include <string>
#include <p8-platform/util/StringUtils.h>

#include "client.h"

#if defined(USE_DBG_CONSOLE) && defined(TARGET_WINDOWS)
int DbgPrintf(const char* szFormat, ...)
{
  static bool g_bDebugConsole = false;
  char szBuffer[4096];
  int nLen;
  va_list args;
  DWORD dwWritten;

  if (!g_bDebugConsole)
  {
    ::AllocConsole();
    g_bDebugConsole = true;
  }

  va_start(args, szFormat);
  nLen = vsnprintf(szBuffer, sizeof(szBuffer) - 1, szFormat, args);
  ::WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szBuffer, nLen, &dwWritten, 0);
  va_end(args);

  return nLen;
}
#endif

bool GetFileContents(const std::string& url, std::string& strContent)
{
  void* fileHandle = g.XBMC->OpenFile(url.c_str(), 0);

  if (fileHandle == NULL)
  {
    KODI_LOG(LOG_ERROR, "GetFileContents: %s failed\n", url.c_str());
    return false;
  }

  char buffer[1024];
  strContent.clear();

  for (;;)
  {
    ssize_t bytesRead = g.XBMC->ReadFile(fileHandle, buffer, sizeof(buffer));
    if (bytesRead <= 0)
      break;
    strContent.append(buffer, bytesRead);
  }

  g.XBMC->CloseFile(fileHandle);

  return true;
}

std::string EncodeURL(const std::string& strUrl)
{
  std::string str;
  for (const auto& c : strUrl)
  {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
      str += c;
    else
    {
      std::string strPercent = StringUtils::Format("%%%02X", (int)c);
      str += strPercent;
    }
  }

  return str;
}

/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Settings.h"

#include <kodi/General.h>
#include <string>

#if defined(TARGET_WINDOWS) && defined(DEBUG)
#define USE_DBG_CONSOLE
#endif

#ifdef USE_DBG_CONSOLE
int DbgPrintf(const char* szFormat, ...);
#else
#define DbgPrintf(...)              do {} while(0)
#endif // USE_DBG_CONSOLE

#define KODI_LOG(level, ...)          \
  do                                  \
  {                                   \
        DbgPrintf("%-10s: ", #level); \
        DbgPrintf(__VA_ARGS__);       \
        DbgPrintf("\n");              \
        if (level > ADDON_LOG_DEBUG || SettingsType::Get().GetDebug())  \
            kodi::Log(level, __VA_ARGS__);        \
  } while (0)

bool GetFileContents(const std::string& url, std::string& strContent);

std::string EncodeURL(const std::string& strUrl);

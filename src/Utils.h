#pragma once
/*
 *      Copyright (C) 2015 Zoltan Csizmadia <zcsizmadia@gmail.com>
 *      https://github.com/zcsizmadia/pvr.hdhomerun
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include <stdlib.h>

#if defined(TARGET_WINDOWS) && defined(DEBUG)
#define USE_DBG_CONSOLE
#endif

#ifdef USE_DBG_CONSOLE
int DbgPrintf(const char* szFormat, ...);
#else
#define DbgPrintf(...)							do {} while(0)
#endif // USE_DBG_CONSOLE

#define KODI_LOG(level, ...)											\
    do																	\
	{																	\
        DbgPrintf("%-10s: ", #level);									\
        DbgPrintf(__VA_ARGS__);											\
        DbgPrintf("\n");												\
        if (g.XBMC && (level > ADDON::LOG_DEBUG || g.Settings.bDebug))  \
            g.XBMC->Log((ADDON::addon_log_t)level, __VA_ARGS__);		\
	} while (0)

#define PVR_STRCPY(dest, source) do { strncpy(dest, source, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0'; } while(0)
#define PVR_STRCLR(dest) memset(dest, 0, sizeof(dest))

bool GetFileContents(const String& url, String& strContent);

String EncodeURL(const String& strUrl);
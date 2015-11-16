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
#include "Utils.h"

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

bool GetFileContents(const String& url, String& strContent)
{
	char buffer[1024];
	void* fileHandle;
	
	strContent.clear();

	fileHandle = g.XBMC->OpenFile(url, 0);

	if (fileHandle == NULL)
	{
		KODI_LOG(0, "GetFileContents: %s failed\n", url.c_str());
		return false;
	}
	
	for (;;)
	{
		int bytesRead = g.XBMC->ReadFile(fileHandle, buffer, sizeof(buffer));
		if (bytesRead <= 0)
			break;
		strContent.append(buffer, bytesRead);
	}

	g.XBMC->CloseFile(fileHandle);

	return true;
}

String EncodeURL(const String& strUrl)
{
	String str, strEsc;

	for (String::const_iterator iter = strUrl.begin(); iter != strUrl.end(); iter++)
	{
		char c = *iter;

		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			str += c;
		else
		{
			String strPercent;
			strPercent.Format("%%%02X", (int)c);
			str += strPercent;
		}
	}

	return str;
}

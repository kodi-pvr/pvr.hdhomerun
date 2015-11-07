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

#include "Utils.h"

int GetFileContents(const CStdString& url, CStdString& strContent)
{
	char buffer[1024];
	void* fileHandle = XBMC->OpenFile(url.c_str(), 0);

	if (fileHandle == NULL)
		return -1;

	strContent.clear();

	while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, sizeof(buffer)))
		strContent.append(buffer, bytesRead);

	XBMC->CloseFile(fileHandle);

	return strContent.length();
}

CStdString EncodeURL(const CStdString& strUrl)
{
	CStdString str, strEsc;

	for (CStdString::const_iterator iter = strUrl.begin(); iter != strUrl.end(); iter++)
	{
		char c = *iter;

		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			str += c;
		else
		{
			char strEsc[5];
			snprintf(strEsc, sizeof(strEsc), "%%%02X", (int)c);
			str += strEsc;
		}
	}

	return str;
}

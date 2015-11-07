#pragma once
/*
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
#include <platform/threads/mutex.h>
#include <hdhomerun/hdhomerun.h>
#include <json/json.h>

class HDHomeRunTuners
{
public:
	enum
	{
		UpdateDiscover = 1,
		UpdateLineUp = 2,
		UpdateGuide = 4
	};

public:
	struct Tuner
	{
		Tuner()
		{
			memset(&Device, 0, sizeof(Device));
		}

		hdhomerun_discover_device_t Device;

		Json::Value LineUp;
		Json::Value Guide;
	};

	typedef std::vector<Tuner> Tuners;

	class AutoLock
	{
	public:
		AutoLock(HDHomeRunTuners* p) : m_p(p) { m_p->Lock(); }
		AutoLock(HDHomeRunTuners& p) : m_p(&p) { m_p->Unlock(); }
		~AutoLock() { m_p->Unlock(); }
	protected:
		HDHomeRunTuners* m_p;
	};

public:
	HDHomeRunTuners();

	bool Update(int nMode = UpdateDiscover | UpdateLineUp | UpdateGuide);

public:
	PVR_ERROR PvrGetChannels(ADDON_HANDLE handle, bool bRadio);
	PVR_ERROR PvrGetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);

protected:
	unsigned int CalculateChannelUniqueId(Json::Value::const_iterator iter);

public:
	void Lock() { m_Lock.Lock(); }
	void Unlock() { m_Lock.Unlock(); }

protected:
	Tuners m_Tuners;
	PLATFORM::CMutex m_Lock;

	int64_t m_LastUpdate;
};

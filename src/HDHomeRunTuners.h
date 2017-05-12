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
#include <p8-platform/threads/mutex.h>
#include <hdhomerun.h>
#include <json/json.h>
#include <cstring>
#include <vector>

namespace PVRHDHomeRun {

class Lockable {
public:
    void LockObject() {
        _lock.Lock();
    }
    void UnlockObject() {
        _lock.Unlock();
    }
private:
    P8PLATFORM::CMutex _lock;
};

class Lock {
public:
    Lock(Lockable* obj) : _obj(obj)
        {
            _obj->LockObject();
        }
    ~Lock()
    {
        _obj->UnlockObject();
    }
private:
    Lockable* _obj;
};

class LineupEntry
{
public:
    String   _guidenumber;
    String   _guidename;
    uint32_t _channel;
    uint32_t _subchannel;
};

class Lineup : public Lockable
{

};

class HDHomeRunTuners : public Lockable
{
public:
    enum
    {
        UpdateDiscover = 1,
        UpdateLineUp   = 2,
        UpdateGuide    = 4
    };

public:
    struct Tuner
    {
        hdhomerun_discover_device_t _discover_device = {0};
        hdhomerun_device_t*         _raw_device      = nullptr;

        Json::Value LineUp;
        Json::Value Guide;
    };

    typedef std::vector<Tuner> Tuners;


public:
    HDHomeRunTuners();

    bool Update(int nMode = UpdateDiscover | UpdateLineUp | UpdateGuide);

public:
    PVR_ERROR PvrGetChannels(ADDON_HANDLE handle, bool bRadio);
    int PvrGetChannelsAmount();
    PVR_ERROR PvrGetEPGForChannel(ADDON_HANDLE handle,
            const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);
    int PvrGetChannelGroupsAmount(void);
    PVR_ERROR PvrGetChannelGroups(ADDON_HANDLE handle, bool bRadio);
    PVR_ERROR PvrGetChannelGroupMembers(ADDON_HANDLE handle,
            const PVR_CHANNEL_GROUP &group);

protected:
    unsigned int PvrCalculateUniqueId(const String& str);



protected:
    Tuners m_Tuners;
};

};

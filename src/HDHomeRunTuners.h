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


class GuideEntry
{
public:
    time_t _starttime;
    time_t _endtime;
    time_t _originalairdate;
    String _title;
    String _episodenumber;
    String _episodetitle;
    String _synopsis;
    String _imageurl;
    String _seriesid;
};

class X {
public:
    String  _guidenumber;
    String  _guidename;
    String  _affiliate;
    String  _imageurl;

};
class LineupEntry
{
public:
    LineupEntry(const Json::Value&);

    String   _guidenumber;
    String   _guidename;
    String   _url;

    uint32_t _channel    = 0;
    uint32_t _subchannel = 0;
    bool     _drm;
};



class Lineup : public Lockable
{
public:
    std::vector<LineupEntry> _entries;
    std::map<std::pair<uint32_t, uint32_t>, LineupEntry> _map;
};

class Tuner : public Lockable
{
public:
    Tuner(const hdhomerun_discover_device_t& d)
        : _debug(hdhomerun_debug_create())
        , _device(hdhomerun_device_create(d.device_id, d.ip_addr, 0, _debug))
        , _discover_device(d) // copy
    {
        _get_api_data();
        _get_discover_data();
        _get_lineup();
    }
    ~Tuner()
    {
        hdhomerun_device_destroy(_device);
        hdhomerun_debug_destroy(_debug);
    }
    void RefreshLineup();

    // Accessors
    unsigned int TunerCount() const
    {
        return _tunercount;
    }
    bool Legacy() const
    {
        return _legacy;
    }
    const String& ChannelMap() const
    {
        return _channelmap;
    }
    const std::vector<LineupEntry>& Lineup() const
    {
        return _lineup;
    }

private:
    void _get_var(String& value, const char* name);
    // Called once
    void _get_api_data();
    // Called multiple times for legacy devices
    void _get_discover_data();
    // Called multiple for all devices
    void _get_lineup();

    // The hdhomerun_... objects depend on the order listed here for proper instantiation.
    hdhomerun_debug_t*          _debug;
    hdhomerun_device_t*         _device;
    hdhomerun_discover_device_t _discover_device;
    // Discover Data
    String                      _lineupURL;
    unsigned int                _tunercount;
    bool                        _legacy;
    // API Data
    String                      _channelmap;
    std::vector<LineupEntry>    _lineup;
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

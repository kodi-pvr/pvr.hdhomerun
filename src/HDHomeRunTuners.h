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
#include "Utils.h"
#include <p8-platform/threads/mutex.h>
#include <hdhomerun.h>
#include <json/json.h>
#include <cstring>
#include <vector>
#include <set>
#include <memory>
#include <string>

namespace PVRHDHomeRun {

template<typename T>
bool operator<(const T& a, const T& b)
{
    return a.operator<(b);
}
template<typename T>
bool operator==(const T& a, const T& b)
{
    return a.operator==(b);
}

class Lockable {
public:
    virtual ~Lockable() {}
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

class GuideNumber
{
public:
    GuideNumber(const Json::Value&);
    GuideNumber(const GuideNumber&) = default;
    virtual ~GuideNumber() = default;

    String _guidenumber;
    String _guidename;

    uint32_t _channel;
    uint32_t _subchannel;

    String toString() const;

    static const uint32_t SubchannelLimit = 10000;
    uint32_t ID() const
    {
        // _subchannel better be below 10000.
        return (_channel * SubchannelLimit) + (_subchannel);
    }

    bool operator<(const GuideNumber&) const;
    bool operator==(const GuideNumber&) const;
};

class GuideEntry
{
public:
    GuideEntry(const Json::Value&);

    time_t _starttime;
    time_t _endtime;
    time_t _originalairdate;
    String _title;
    String _episodenumber;
    String _episodetitle;
    String _synopsis;
    String _imageURL;
    String _seriesID;

    bool operator<(const GuideEntry& rhs) const
    {
        return _starttime < rhs._starttime;
    }
    bool operator==(const GuideEntry& rhs) const
    {
        return _starttime == rhs._starttime;
    }
};


class Guide
{
public:
    Guide(const Json::Value&);

    String               _affiliate;
    String               _imageurl;
    std::set<GuideEntry> _entries;
};

class Tuner
{
public:
    Tuner(const hdhomerun_discover_device_t& d);
    Tuner(const Tuner&) = delete;
    Tuner(Tuner&&) = default;
    ~Tuner();
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
    const uint32_t DeviceID() const
    {
        return _discover_device.device_id;
    }

private:
    void _get_var(String& value, const char* name);
    // Called once
    void _get_api_data();
    // Called multiple times for legacy devices
    void _get_discover_data();

    // The hdhomerun_... objects depend on the order listed here for proper instantiation.
    hdhomerun_debug_t*          _debug;
    hdhomerun_device_t*         _device;
    hdhomerun_discover_device_t _discover_device;
    // Discover Data
    String                      _lineupURL;
    unsigned int                _tunercount;
    bool                        _legacy;
public:
    bool operator<(const Tuner& rhs) const
    {
        return DeviceID() < rhs.DeviceID();
    }
    friend class Lineup;
};

class Info
{
public:
    Info(const Json::Value&);
    Info() = default;

    String   _url;
    bool     _drm = false;;

    // Tuners which can receive this channel.
    // Entries are owned by Lineup
    std::set<Tuner*> _tuners;
};

class Lineup : public Lockable
{
public:
    Lineup();
    ~Lineup() {}

    void DiscoverTuners();
    void UpdateLineup();
    void Update();

    PVR_ERROR PvrGetChannels(ADDON_HANDLE handle, bool bRadio);
    int PvrGetChannelsAmount();
    PVR_ERROR PvrGetEPGForChannel(ADDON_HANDLE handle,
            const PVR_CHANNEL& channel,
            time_t iStart,
            time_t iEnd);
    int PvrGetChannelGroupsAmount(void);
    PVR_ERROR PvrGetChannelGroups(ADDON_HANDLE handle, bool bRadio);
    PVR_ERROR PvrGetChannelGroupMembers(ADDON_HANDLE handle,
            const PVR_CHANNEL_GROUP &group);

private:
    std::set<Tuner*>             _tuners;
    std::set<uint32_t>           _device_ids;
    std::set<GuideNumber>        _lineup;
    std::map<GuideNumber, Info>  _info;
    std::map<GuideNumber, Guide> _guide;
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

/*
 *      Copyright (C) 2017 Matthew Lundberg <matthew.k.lundberg@gmail.com>
 *      https://github.com/MatthewLundberg/pvr.hdhomerun
 *
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
#include "HDHomeRunTuners.h"
#include <functional>
#include <algorithm>
#include <iterator>
#include <string>

using namespace ADDON;

namespace PVRHDHomeRun {

bool operator==(const String& a, const String& b)
{
    return !strcmp(a.c_str(), b.c_str());
}
bool operator==(const String& a, const char* b)
{
    return !strcmp(a.c_str(), b);
}


GuideNumber::GuideNumber(const Json::Value& v)
{
    _guidenumber = v["GuideNumber"].asString();
    _guidename   = v["GuideName"].asString();

     _channel = atoi(_guidenumber.c_str());
     auto dot = _guidenumber.find('.');
     if (std::string::npos != dot)
     {
         _subchannel = atoi(_guidenumber.c_str() + dot + 1);
     }
     else
     {
         _subchannel = 0;
     }
}

bool GuideNumber::operator<(const GuideNumber& rhs) const
{
    if (_channel < rhs._channel)
    {
        return true;
    }
    else if (_channel == rhs._channel)
    {
        if (_subchannel < rhs._subchannel)
        {
            return true;
        }
    }

    return false;
}
bool GuideNumber::operator==(const GuideNumber& rhs) const
{
    bool ret = (_channel == rhs._channel)
            && (_subchannel == rhs._subchannel)
            ;
    return ret;
}
String GuideNumber::toString() const
{
    char channel[64];
    sprintf(channel, "%d.%d", _channel, _subchannel);
    return String("") + channel + " "
            + "_guidename("   + _guidename   + ") ";
}

template<typename T>
unsigned int GetGenreType(const T& arr)
{
    unsigned int nGenreType = 0;

    for (auto& i : arr)
    {
        auto str = i.asString();

        if (str == "News")
            nGenreType |= EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
        else if (str == "Comedy")
            nGenreType |= EPG_EVENT_CONTENTMASK_SHOW;
        else if (str == "Movie" || str == "Drama")
            nGenreType |= EPG_EVENT_CONTENTMASK_MOVIEDRAMA;
        else if (str == "Food")
            nGenreType |= EPG_EVENT_CONTENTMASK_LEISUREHOBBIES;
        else if (str == "Talk Show")
            nGenreType |= EPG_EVENT_CONTENTMASK_SHOW;
        else if (str == "Game Show")
            nGenreType |= EPG_EVENT_CONTENTMASK_SHOW;
        else if (str == "Sport" || str == "Sports")
            nGenreType |= EPG_EVENT_CONTENTMASK_SPORTS;
    }

    return nGenreType;
}


GuideEntry::GuideEntry(const Json::Value& v)
{
    _starttime       = v["StartTime"].asUInt();
    _endtime         = v["EndTime"].asUInt();
    _originalairdate = v["OriginalAirdate"].asUInt();
    _title           = v["Title"].asString();
    _episodenumber   = v["EpisodeNumber"].asString();
    _synopsis        = v["Synopsis"].asString();
    _imageURL        = v["ImageURL"].asString();
    _seriesID        = v["SeriesID"].asString();
    _genre           = GetGenreType(v["Filter"]);
}

Guide::Guide(const Json::Value& v)
{
    _affiliate = v["Affiliate"].asString();
    _imageURL  = v["ImageURL"].asString();
}

Info::Info(const Json::Value& v)
{
    _url = v["URL"].asString();
    _drm = v["DRM"].asBool();
    _hd  = v["HD"].asBool();

     //KODI_LOG(LOG_DEBUG, "LineupEntry::LineupEntry %s", toString().c_str());
}


Tuner::Tuner(const hdhomerun_discover_device_t& d)
    : _debug(nullptr) // _debug(hdhomerun_debug_create())
    , _device(hdhomerun_device_create(d.device_id, d.ip_addr, 0, _debug))
    , _discover_device(d) // copy
{
    _get_api_data();
    _get_discover_data();
}

Tuner::~Tuner()
{
    hdhomerun_device_destroy(_device);
    //hdhomerun_debug_destroy(_debug);
}


void Tuner::_get_var(String& value, const char* name)
{
    char *get_val;
    char *get_err;
    if (hdhomerun_device_get_var(_device, name, &get_val, &get_err) < 0)
    {
        KODI_LOG(LOG_ERROR,
                "communication error sending %s request to %08x",
                name, _discover_device.device_id
        );
    }
    else if (get_err)
    {
        KODI_LOG(LOG_ERROR, "error %s with %s request from %08x",
                get_err, name, _discover_device.device_id
        );
    }
    else
    {
        KODI_LOG(LOG_DEBUG, "Success getting value %s = %s from %08x",
                name, get_val,
                _discover_device.device_id
        );

        value.assign(get_val);
    }
}
void Tuner::_set_var(const char* value, const char* name)
{
    char* set_err;
    if (hdhomerun_device_set_var(_device, name, value, NULL, &set_err) < 0)
    {
        KODI_LOG(LOG_ERROR,
                "communication error sending set request %s = %s to %08x",
                name, value,
                _discover_device.device_id
        );
    }
    else if (set_err)
    {
        KODI_LOG(LOG_ERROR, "error %s with %s = %s request from %08x",
                set_err,
                name, value,
                _discover_device.device_id
        );
    }
    else
    {
        KODI_LOG(LOG_DEBUG, "Success setting value %s = %s from %08x",
                name, value,
                _discover_device.device_id
        );
    }
}
void Tuner::_get_api_data()
{
}

void Tuner::_get_discover_data()
{
    String discoverURL;
    String discoverResults;

    // Ask the device for its lineup URL
    discoverURL.Format("%s/discover.json", _discover_device.base_url);
    if (GetFileContents(discoverURL, discoverResults))
    {
        Json::Reader jsonReader;
        Json::Value discoverJson;
        if (jsonReader.parse(discoverResults, discoverJson))
        {
            auto& lineupURL  = discoverJson["LineupURL"];
            auto& tunercount = discoverJson["TunerCount"];
            auto& legacy     = discoverJson["Legacy"];

            _lineupURL  = std::move(lineupURL.asString());
            _tunercount = std::move(tunercount.asUInt());
            _legacy     = std::move(legacy.asBool());

            KODI_LOG(LOG_DEBUG, "HDR ID %08x LineupURL %s Tuner Count %d Legacy %d",
                    _discover_device.device_id,
                    _lineupURL.c_str(),
                    _tunercount,
                    _legacy
            );
        }
    }
    else
    {
        // Fall back to a pattern for "modern" devices
        KODI_LOG(LOG_DEBUG, "HDR ID %08x Fallback lineup URL %s/lineup.json",
                _discover_device.device_id,
                _discover_device.base_url
        );

        _lineupURL.Format("%s/lineup.json", _discover_device.base_url);
    }
}

void Tuner::RefreshLineup()
{
    if (_legacy)
    {
        _get_discover_data();
    }
}

void Lineup::DiscoverTuners()
{
    struct hdhomerun_discover_device_t discover_devices[64];
    size_t tuner_count = hdhomerun_discover_find_devices_custom_v2(
            0,
            HDHOMERUN_DEVICE_TYPE_TUNER,
            HDHOMERUN_DEVICE_ID_WILDCARD,
            discover_devices,
            64
            );

    std::set<uint32_t> discovered_ids;

    bool tuner_added   = false;
    bool tuner_removed = false;

    Lock lock(this);
    for (size_t i=0; i<tuner_count; i++)
    {
        auto& dd = discover_devices[i];
        auto  id = dd.device_id;

        if (dd.is_legacy && !g.Settings.bUseLegacy)
            continue;

        discovered_ids.insert(id);

        if (_device_ids.find(id) == _device_ids.end())
        {
            // New tuner
            tuner_added = true;
            KODI_LOG(LOG_DEBUG, "Adding tuner %08x", id);

            _tuners.insert(new Tuner(dd));
            _device_ids.insert(id);
        }
    }

    // Iterate through tuners, determine if there are stale entries.
    for (auto tuner : _tuners)
    {
        uint32_t id = tuner->DeviceID();
        if (discovered_ids.find(id) == discovered_ids.end())
        {
            // Tuner went away
            tuner_removed = true;
            KODI_LOG(LOG_DEBUG, "Removing tuner %08x", id);

            auto ptuner = const_cast<Tuner*>(tuner);

            for (auto number : _lineup)
            {
                auto info = _info[number];
                auto tuners = info._tuners;

                if (tuners.find(ptuner) != tuners.end())
                {
                    // Remove tuner from lineup
                    KODI_LOG(LOG_DEBUG, "Removing tuner from GuideNumber %s", number.toString().c_str());
                    info.RemoveTuner(ptuner);
                }
                if (tuners.size() == 0)
                {
                    // No tuners left for this lineup guide entry, remove it

                    KODI_LOG(LOG_DEBUG, "No tuners left, removing GuideNumber %s", number.toString().c_str());
                    _lineup.erase(number);
                    _info.erase(number);
                    _guide.erase(number);
                }
            }

            // Erase tuner from this
            _tuners.erase(tuner);
            _device_ids.erase(id);
            delete(tuner);
        }
    }

    if (tuner_added) {
        // TODO - check lineup, add new tuner to lineup entries, might create new lineup entries for this tuner.

        UpdateLineup();
        UpdateGuide();
    }
    if (tuner_removed) {
        // TODO - Lineup should be correct, anything to do?
    }
}

void Lineup::UpdateLineup()
{
    KODI_LOG(LOG_DEBUG, "Lineup::UpdateLineup");

    Lock lock(this);
    _lineup.clear();

    for (auto tuner: _tuners)
    {

        KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun channel lineup for %08x: %s",
                tuner->_discover_device.device_id, tuner->_lineupURL.c_str()
        );

        String lineupStr;
        if (!GetFileContents(tuner->_lineupURL, lineupStr))
        {
            KODI_LOG(LOG_ERROR, "Cannot get lineup from %s", tuner->_lineupURL.c_str());
            continue;
        }

        Json::Value lineupJson;
        Json::Reader jsonReader;
        if (!jsonReader.parse(lineupStr, lineupJson))
        {
            KODI_LOG(LOG_ERROR, "Cannot parse JSON value returned from %s", tuner->_lineupURL.c_str());
            continue;
        }

        if (lineupJson.type() != Json::arrayValue)
        {
            KODI_LOG(LOG_ERROR, "Lineup is not a JSON array, returned from %s", tuner->_lineupURL.c_str());
            continue;
        }

        auto ptuner = const_cast<Tuner*>(tuner);
        for (auto& v : lineupJson)
        {
            // TODO Check here for g.Settings.bAllowUnknownChannels
            GuideNumber number = v;
            if ((!g.Settings.bAllowUnknownChannels) && (number._guidename == "Unknown"))
            {
                continue;
            }
            _lineup.insert(number);
            if (_info.find(number) == _info.end())
            {
                Info info = v;
                _info[number] = info;
            }
            _info[number].AddTuner(ptuner);
        }
    }

    for (const auto& number: _lineup)
    {
        String tuners;
        auto& info = _info[number];

        for (auto tuner : info._tuners)
        {
            char id[10];
            sprintf(id, " %08x", tuner->DeviceID());
            tuners += id;
        }
        KODI_LOG(LOG_DEBUG,
                "Lineup Entry: %d.%d - %s - %s - %s",
                number._channel,
                number._subchannel,
                number._guidenumber.c_str(),
                number._guidename.c_str(),
                tuners.c_str()
        );
    }
}

// Increment the first element until max is reached, then increment further indices.
bool increment_index(
        std::vector<size_t>::iterator index,
        std::vector<size_t>::iterator end,
        size_t max)
{
    if (index == end) {
        return false;
    }

    (*index) ++;
    if ((*index) >= max)
    {
        if (!increment_index(index+1, end, max-1))
        {
            return false;
        }
        (*index) = (*(index+1)) + 1;
    }
    return true;
}

void Lineup::UpdateGuide()
{
    // Find a minimal covering of the lineup

    Lock lock(this);

    // Create vector of tuners for channel search
    std::vector<Tuner*> tuners;
    std::copy(_tuners.begin(), _tuners.end(), std::back_inserter(tuners));

    std::vector<size_t> index;
    bool matched;
    for (int num_tuners = 1; num_tuners <= tuners.size(); num_tuners ++)
    {
        // Create index, reverse order
        index.clear();
        for (size_t i=0; i<num_tuners; i++)
        {
            index.insert(index.begin(), i);
        }

        matched = true;
        do {
            // Unique combination of tuners in index
            matched = true;
            for (auto& number: _lineup)
            {
                auto& info = _info[number];
                auto& info_tuners = info._tuners;

                bool tunermatch = false;
                for (auto idx: index) {
                    auto tuner = tuners[idx];

                    if (info_tuners.find(tuner) != info_tuners.end())
                    {
                        tunermatch = true;
                        break;
                    }
                }
                if (!tunermatch)
                {
                    matched = false;
                    break;
                }
            }

        } while (!matched && increment_index(index.begin(), index.end(), tuners.size()));
        if (matched)
            break;
    }
    if (!matched)
    {
        return;
    }
    String idx;
    for (auto& i : index) {
        char buf[10];
        sprintf(buf, " %08x", tuners[i]->DeviceID());
        idx += buf;
    }
    KODI_LOG(LOG_DEBUG, "UpateGuide - Need to scan %u tuners - %s", index.size(), idx.c_str());

    //return; // TODO - remove

    for (auto idx: index) {
        auto tuner = tuners[idx];

        String URL;
        URL.Format(
                "http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s",
                EncodeURL(tuner->Auth()).c_str()
        );
        KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun guide for %08x: %s",
                tuner->DeviceID(), URL.c_str());

        String guidedata;
        if (!GetFileContents(URL.c_str(), guidedata))
        {
            KODI_LOG(LOG_ERROR, "Error requesting guide for %08x from %s",
                    tuner->DeviceID(), URL.c_str());
            continue;
        }

        Json::Reader jsonreader;
        Json::Value  jsontunerguide;
        if (!jsonreader.parse(guidedata, jsontunerguide))
        {
            KODI_LOG(LOG_ERROR, "Error parsing JSON guide data for %08x", tuner->DeviceID());
            continue;
        }
        if (jsontunerguide.type() != Json::arrayValue)
        {
            KODI_LOG(LOG_ERROR, "Top-level JSON guide data is not an array for %08x", tuner->DeviceID());
            continue;
        }

        for (auto& jsonchannelguide : jsontunerguide)
        {
            GuideNumber number = jsonchannelguide;

            if (_guide.find(number) == _guide.end())
            {
                KODI_LOG(LOG_DEBUG, "Inserting guide for channel %u", number.ID());
                _guide[number] = jsonchannelguide;
            }

            Guide& channelguide = _guide[number];

            auto jsonguidenetries = jsonchannelguide["Guide"];
            if (jsonguidenetries.type() != Json::arrayValue)
            {
                KODI_LOG(LOG_ERROR, "Guide entries is not an array for %08x", tuner->DeviceID());
                continue;
            }
            for (auto& jsonentry: jsonguidenetries)
            {
                channelguide.InsertEntry(jsonentry);
            }

            // TODO age out old entries
        }
    }

}

int Lineup::PvrGetChannelsAmount()
{
    return _lineup.size();
}
PVR_ERROR Lineup::PvrGetChannels(ADDON_HANDLE handle, bool radio)
{
    if (radio)
        return PVR_ERROR_NO_ERROR;

    Lock lock(this);
    for (auto& number: _lineup)
    {
        PVR_CHANNEL pvrChannel = {0};
        auto& guide = _guide[number];

        pvrChannel.iUniqueId         = number.ID();
        pvrChannel.iChannelNumber    = number._channel;
        pvrChannel.iSubChannelNumber = number._subchannel;

        const String* name = &number._guidename;
        if (guide._affiliate.length())
            name = &guide._affiliate;
        PVR_STRCPY(pvrChannel.strChannelName, name->c_str());

        PVR_STRCPY(pvrChannel.strIconPath, guide._imageURL.c_str());

        sprintf(pvrChannel.strStreamURL, "pvr://stream/%d", number.ID());

        g.PVR->TransferChannelEntry(handle, &pvrChannel);
    }
    return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Lineup::PvrGetEPGForChannel(ADDON_HANDLE handle,
        const PVR_CHANNEL& channel, time_t start, time_t end
        )
{
    KODI_LOG(LOG_DEBUG,
            "PvrGetEPCForChannel Handle:%p Channel ID: %d Number: %u Sub: %u Start: %u End: %u",
            handle,
            channel.iUniqueId,
            channel.iChannelNumber,
            channel.iSubChannelNumber,
            start,
            end
            );

    Lock lock(this);


    auto& info  = _info[channel.iUniqueId];
    auto& guide = _guide[channel.iUniqueId];
    for (auto& ge: guide._entries)
    {
        if (ge._endtime < start)
            continue;
        if (ge._starttime > end)
            break;

        EPG_TAG tag = {0};

        tag.iUniqueBroadcastId = ge._id;
        tag.strTitle           = ge._title.c_str();
        tag.iChannelNumber     = channel.iUniqueId;
        tag.startTime          = ge._starttime;
        tag.endTime            = ge._endtime;
        tag.firstAired         = ge._originalairdate;
        tag.strPlot            = ge._synopsis;
        tag.strIconPath        = ge._imageURL;
        //tag.iSeriesNumber
        //tag.iEpisodeNumber
        tag.iGenreType         = ge._genre;

        g.PVR->TransferEpgEntry(handle, &tag);
    }

    return PVR_ERROR_NO_ERROR;
}

int Lineup::PvrGetChannelGroupsAmount()
{
    return 3;
}

static const String FavoriteChannels = "Favorite channels";
static const String HDChannels       = "HD channels";
static const String SDChannels       = "SD channels";


PVR_ERROR Lineup::PvrGetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
    PVR_CHANNEL_GROUP channelGroup;

    if (bRadio)
        return PVR_ERROR_NO_ERROR;

    memset(&channelGroup, 0, sizeof(channelGroup));

    channelGroup.iPosition = 1;
    PVR_STRCPY(channelGroup.strGroupName, FavoriteChannels.c_str());
    g.PVR->TransferChannelGroup(handle, &channelGroup);

    channelGroup.iPosition++;
    PVR_STRCPY(channelGroup.strGroupName, HDChannels.c_str());
    g.PVR->TransferChannelGroup(handle, &channelGroup);

    channelGroup.iPosition++;
    PVR_STRCPY(channelGroup.strGroupName, SDChannels.c_str());
    g.PVR->TransferChannelGroup(handle, &channelGroup);

    return PVR_ERROR_NO_ERROR;
}

PVR_ERROR Lineup::PvrGetChannelGroupMembers(ADDON_HANDLE handle,
        const PVR_CHANNEL_GROUP &group)
{
    Lock lock(this);

    for (const auto& number: _lineup)
    {
        auto& info  = _info[number];
        auto& guide = _guide[number];

        if ((FavoriteChannels != group.strGroupName) && !info._favorite)
            continue;
        if ((HDChannels != group.strGroupName) && !info._hd)
            continue;
        if ((SDChannels != group.strGroupName) && info._hd)
            continue;

        PVR_CHANNEL_GROUP_MEMBER channelGroupMember = {0};
        PVR_STRCPY(channelGroupMember.strGroupName, group.strGroupName);
        channelGroupMember.iChannelUniqueId = number.ID();
        //channelGroupMember.iChannelUniqueId = number._channel;

        g.PVR->TransferChannelGroupMember(handle, &channelGroupMember);
    }
    return PVR_ERROR_NO_ERROR;
}

Tuner* Info::GetNextTuner()
{
    if (_has_next)
    {
        _next ++;
        if (_next == _tuners.end())
        {
            _has_next = false;
            return nullptr;
        }
    }
    else
    {
        _has_next = true;
        _next = _tuners.begin();
    }
    return *_next;
}
void Info::ResetNextTuner()
{
    _has_next = false;
}
void Info::AddTuner(Tuner* t)
{
    _tuners.insert(t);
    ResetNextTuner();
}
void Info::RemoveTuner(Tuner* t)
{
    _tuners.erase(t);
    ResetNextTuner();
}

const char* Lineup::GetLiveStreamURL(const PVR_CHANNEL& channel)
{
    Lock lock(this);
    std::cout << "Lineup::GetLiveStreamURL " << channel.iUniqueId << "\n";

    auto  id    = channel.iUniqueId;
    const auto& entry = _lineup.find(id);
    if (entry == _lineup.end())
    {
        std::cout << "Not found\n";
        KODI_LOG(LOG_ERROR, "Channel %d not found!", id);
        return "";
    }
    auto& info = _info[id];

    Tuner* tuner = nullptr;

    int pass;
    do {
        tuner = info.GetNextTuner();
        if (tuner)
            break;
        pass ++;
    } while (pass < 2);

    std::cout << std::hex << tuner->DeviceID() << " " << tuner->IP() << std::dec << "\n";

    char cstr[32];
    if (channel.iSubChannelNumber)
    {
        sprintf(cstr, "%d.%d", channel.iChannelNumber, channel.iSubChannelNumber);
    }
    else
    {
        sprintf(cstr, "%d",    channel.iChannelNumber);
    }
    static char buf[1024];
    sprintf(buf, "http://%s:5004/auto/v%s", tuner->IP().c_str(), cstr);

    std::cout << buf << "\n";

    return buf;
}
}; // namespace PVRHDHomeRun

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
#include "HDHomeRunTuners.h"
#include <functional>
#include <algorithm>
#include <iterator>
#include <string>

using namespace ADDON;

namespace PVRHDHomeRun {


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

     //KODI_LOG(LOG_DEBUG, "LineupEntry::LineupEntry %s", toString().c_str());
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
        else if (_subchannel == rhs._subchannel)
        {
            if (strcmp(_guidename.c_str(), rhs._guidename.c_str()) < 0)
            {
                return true;
            }
        }
    }

    return false;
}
bool GuideNumber::operator==(const GuideNumber& rhs) const
{
    bool ret = (_channel == rhs._channel)
            && (_subchannel == rhs._subchannel)
            && !strcmp(_guidename.c_str(), rhs._guidename.c_str())
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
        KODI_LOG(LOG_DEBUG, "channelmap(%08x) = %s",
                _discover_device.device_id, get_val
        );

        value.assign(get_val);
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
                    tuners.erase(ptuner);
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
            if ((!g.Settings.bAllowUnknownChannels) && (!strcmp(number._guidename.c_str(), "Unknown")))
            {
                continue;
            }
            _lineup.insert(number);
            if (_info.find(number) == _info.end())
            {
                Info info = v;
                _info[number] = info;
            }
            _info[number]._tuners.insert(ptuner);
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
            continue;
        }
        Json::Reader jsonreader;
        Json::Value  jsontunerguide;
        if (!jsonreader.parse(guidedata, jsontunerguide))
        {
            continue;
        }
        if (jsontunerguide.type() != Json::arrayValue)
        {
            continue;
        }

        for (auto& jsonchannelguide : jsontunerguide)
        {
            GuideNumber number       = jsonchannelguide;
            if (_guide.find(number) == _guide.end())
            {
                _guide[number] = jsonchannelguide;
            }
            Guide channelguide = _guide[number];

            auto jsonguidenetries = jsonchannelguide["Guide"];
            if (jsonguidenetries.type() != Json::arrayValue)
            {
                continue;
            }
            for (auto& jsonentry: jsonguidenetries)
            {
                channelguide._entries.insert(jsonentry);
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

        pvrChannel.iUniqueId         = number.ID();
        pvrChannel.iChannelNumber    = number._channel;
        pvrChannel.iSubChannelNumber = number._subchannel;
        PVR_STRCPY(pvrChannel.strChannelName, number._guidename.c_str());
        PVR_STRCPY(pvrChannel.strStreamURL, "");

        auto& guide = _guide[number];
        PVR_STRCPY(pvrChannel.strIconPath, guide._imageURL.c_str());

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


}

unsigned int HDHomeRunTuners::PvrCalculateUniqueId(const String& str)
{
    int nHash = (int) std::hash<std::string>()(str);
    return (unsigned int) abs(nHash);
}

bool HDHomeRunTuners::Update(int nMode)
{
    struct hdhomerun_discover_device_t foundDevices[16];
    Json::Value::ArrayIndex nIndex, nCount, nGuideIndex;
    int nTunerCount, nTunerIndex;
    String strUrl, strJson;
    Json::Reader jsonReader;
    Json::Value jsonResponse;
    Tuner* pTuner;
    std::set<String> guideNumberSet;

    //
    // Discover
    //

    nTunerCount = hdhomerun_discover_find_devices_custom_v2(0,
            HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD,
            foundDevices, 16);

    KODI_LOG(LOG_DEBUG, "Found %d HDHomeRun tuners", nTunerCount);

    Lock lock(this);

    if (nMode & UpdateDiscover)
        m_Tuners.clear();

    if (nTunerCount <= 0)
        return false;

    for (nTunerIndex = 0; nTunerIndex < nTunerCount; nTunerIndex++)
    {
        pTuner = NULL;

        if (nMode & UpdateDiscover)
        {
            // New device
            Tuner tuner;
            pTuner = &*m_Tuners.insert(m_Tuners.end(), tuner);
        }
        else
        {
            // Find existing device
            for (Tuners::iterator iter = m_Tuners.begin();
                    iter != m_Tuners.end(); iter++)
                if (iter->_discover_device.ip_addr == foundDevices[nTunerIndex].ip_addr)
                {
                    pTuner = &*iter;
                    break;
                }
        }

        if (pTuner == NULL)
            continue;

        //
        // Update device
        //
        pTuner->_discover_device = foundDevices[nTunerIndex];

        //
        // Guide
        //

        if (nMode & UpdateGuide)
        {

            // TODO - remove logging

            hdhomerun_discover_device_t *discover_dev = &pTuner->_discover_device;
            KODI_LOG(LOG_DEBUG, "hdhomerun_discover_device_t %p", discover_dev);
            KODI_LOG(LOG_DEBUG, "IP:    %08x", discover_dev->ip_addr);
            KODI_LOG(LOG_DEBUG, "Type:  %08x", discover_dev->device_type);
            KODI_LOG(LOG_DEBUG, "ID:    %08x", discover_dev->device_id);
            KODI_LOG(LOG_DEBUG, "Tuners: %u", discover_dev->tuner_count);
            KODI_LOG(LOG_DEBUG, "Legacy: %u", discover_dev->is_legacy);
            KODI_LOG(LOG_DEBUG, "Auth:   %24s", discover_dev->device_auth);
            KODI_LOG(LOG_DEBUG, "URL:    %28s", discover_dev->base_url);

            hdhomerun_debug_t *dbg = hdhomerun_debug_create();
            auto hdr_dev = pTuner->_raw_device = hdhomerun_device_create(
                    discover_dev->device_id, discover_dev->ip_addr, 0, dbg);

            KODI_LOG(LOG_DEBUG, "hdhomerun_device_t %p", hdr_dev);

            char *get_val;
            char *get_err;
            if (hdhomerun_device_get_var(hdr_dev, "/tuner0/channelmap",
                    &get_val, &get_err) < 0)
            {
                KODI_LOG(LOG_DEBUG,
                        "communication error sending channelmap request to %p",
                        hdr_dev);
            }
            else if (get_err)
            {
                KODI_LOG(LOG_DEBUG, "error %s with channelmap request from %p",
                        get_err, hdr_dev);
            }
            else
            {
                KODI_LOG(LOG_DEBUG, "channelmap(%p) = %s", hdr_dev, get_val);
            }

            hdhomerun_debug_destroy(dbg);

            strUrl.Format("http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s",
                    EncodeURL(pTuner->_discover_device.device_auth).c_str());

            KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun guide for %08x: %s",
                    pTuner->_discover_device.device_id, strUrl.c_str());

            if (GetFileContents(strUrl.c_str(), strJson))
                if (jsonReader.parse(strJson, pTuner->Guide)
                        && pTuner->Guide.type() == Json::arrayValue)
                {
                    for (nIndex = 0, nCount = 0; nIndex < pTuner->Guide.size();
                            nIndex++)
                    {
                        Json::Value& jsonGuide = pTuner->Guide[nIndex]["Guide"];
                        if (jsonGuide.type() != Json::arrayValue)
                            continue;

                        for (Json::Value::ArrayIndex i = 0;
                                i < jsonGuide.size(); i++, nCount++)
                        {
                            Json::Value& jsonGuideItem = jsonGuide[i];
                            int iSeriesNumber = 0, iEpisodeNumber = 0;

                            jsonGuideItem["_UID"] =
                                    PvrCalculateUniqueId(
                                            jsonGuideItem["Title"].asString()
                                                    + jsonGuideItem["EpisodeNumber"].asString()
                                                    + jsonGuideItem["ImageURL"].asString());

                            if (g.Settings.bMarkNew
                                    && jsonGuideItem["OriginalAirdate"].asUInt()
                                            != 0
                                    && jsonGuideItem["OriginalAirdate"].asUInt()
                                            + 48 * 60 * 60
                                            > jsonGuideItem["StartTime"].asUInt())
                            {
                                jsonGuideItem["Title"] = "*"
                                        + jsonGuideItem["Title"].asString();
                            }

                            Json::Value& jsonFilter = jsonGuideItem["Filter"];
                            unsigned int nGenreType = GetGenreType(jsonFilter);
                            jsonGuideItem["_GenreType"] = nGenreType;

                            if (sscanf(
                                    jsonGuideItem["EpisodeNumber"].asString().c_str(),
                                    "S%dE%d", &iSeriesNumber, &iEpisodeNumber)
                                    != 2)
                                if (sscanf(
                                        jsonGuideItem["EpisodeNumber"].asString().c_str(),
                                        "EP%d", &iEpisodeNumber) == 1)
                                    iSeriesNumber = 0;

                            jsonGuideItem["_SeriesNumber"] = iSeriesNumber;
                            jsonGuideItem["_EpisodeNumber"] = iEpisodeNumber;
                        }
                    }

                    KODI_LOG(LOG_DEBUG, "Found %u guide entries", nCount);
                }
                else
                {
                    KODI_LOG(LOG_ERROR, "Failed to parse guide",
                            strUrl.c_str());
                }
        }

        //
        // Lineup
        //

        if (nMode & UpdateLineUp)
        {
            // Find URL in the discovery data
            hdhomerun_discover_device_t *discover_dev = &pTuner->_discover_device;
            String sDiscoverUrl;
            sDiscoverUrl.Format("%s/discover.json", discover_dev->base_url);
            if (GetFileContents(sDiscoverUrl.c_str(), strJson))
            {
                Json::Value jDiscover;
                if (jsonReader.parse(strJson, jDiscover))
                {
                    Json::Value& lineup = jDiscover["LineupURL"];
                    strUrl.assign(lineup.asString());
                }
            }
            else
            {
                // Fall back to a pattern
                strUrl.Format("%s/lineup.json", pTuner->_discover_device.base_url);
            }

            KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun lineup for %08x: %s",
                    pTuner->_discover_device.device_id, strUrl.c_str());

            if (GetFileContents(strUrl.c_str(), strJson))
            {
                if (jsonReader.parse(strJson, pTuner->LineUp)
                        && pTuner->LineUp.type() == Json::arrayValue)
                {
                    int nChannelNumber = 1;

                    // TODO remove hack
                    // Print the device ID with the channel name in the Kodi display
                    char device_id_s[10] = "";
                    sprintf(device_id_s, " %08x", pTuner->_discover_device.device_id);

                    for (nIndex = 0; nIndex < pTuner->LineUp.size(); nIndex++)
                    {
                        Json::Value& jsonChannel = pTuner->LineUp[nIndex];
                        bool bHide;

                        bHide =
                                ((jsonChannel["DRM"].asBool()
                                        && g.Settings.bHideProtected)
                                        || (g.Settings.bHideDuplicateChannels
                                                && guideNumberSet.find(
                                                        jsonChannel["GuideNumber"].asString())
                                                        != guideNumberSet.end()));

                        jsonChannel["_UID"] = PvrCalculateUniqueId(
                                jsonChannel["GuideName"].asString()
                                        + jsonChannel["URL"].asString());
                        jsonChannel["_ChannelName"] =
                                jsonChannel["GuideName"].asString()
                                        + device_id_s;

                        // Find guide entry
                        for (nGuideIndex = 0;
                                nGuideIndex < pTuner->Guide.size();
                                nGuideIndex++)
                        {
                            const Json::Value& jsonGuide =
                                    pTuner->Guide[nGuideIndex];
                            if (jsonGuide["GuideNumber"].asString()
                                    == jsonChannel["GuideNumber"].asString())
                            {
                                if (jsonGuide["Affiliate"].asString() != "")
                                    jsonChannel["_ChannelName"] =
                                            jsonGuide["Affiliate"].asString()
                                                    + device_id_s;
                                jsonChannel["_IconPath"] =
                                        jsonGuide["ImageURL"].asString();
                                break;
                            }
                        }

                        jsonChannel["_Hide"] = bHide;

                        if (bHide)
                        {
                            jsonChannel["_ChannelNumber"] = 0;
                            jsonChannel["_SubChannelNumber"] = 0;
                        }
                        else
                        {
                            int nChannel = 0, nSubChannel = 0;
                            if (sscanf(
                                    jsonChannel["GuideNumber"].asString().c_str(),
                                    "%d.%d", &nChannel, &nSubChannel) != 2)
                            {
                                nSubChannel = 0;
                                if (sscanf(
                                        jsonChannel["GuideNumber"].asString().c_str(),
                                        "%d", &nChannel) != 1)
                                    nChannel = nChannelNumber;
                            }
                            jsonChannel["_ChannelNumber"] = nChannel;
                            jsonChannel["_SubChannelNumber"] = nSubChannel;
//              guideNumberSet.insert(jsonChannel["GuideNumber"].asString());

                            nChannelNumber++;
                        }
                    }

                    KODI_LOG(LOG_DEBUG, "Found %u channels for %08x",
                            pTuner->LineUp.size(), pTuner->_discover_device.device_id);
                }
                else
                {
                    KODI_LOG(LOG_ERROR,
                            "Failed to parse lineup from %s for %08x",
                            strUrl.c_str(), pTuner->_discover_device.device_id);
                }
            }
        }
    }

    return true;
}



PVR_ERROR HDHomeRunTuners::PvrGetEPGForChannel(ADDON_HANDLE handle,
        const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{

    KODI_LOG(LOG_DEBUG, "PvrGetEPCForChannel Handle:%p Channel ID: %08x Number: %u Sub: %u Start: %u End: %u",
            handle,
            channel.iUniqueId,
            channel.iChannelNumber,
            channel.iSubChannelNumber,
            iStart,
            iEnd
            );

    Json::Value::ArrayIndex nChannelIndex, nGuideIndex;

    Lock lock(this);

    for (Tuners::const_iterator iterTuner = m_Tuners.begin();
            iterTuner != m_Tuners.end(); iterTuner++)
    {
        for (nChannelIndex = 0; nChannelIndex < iterTuner->LineUp.size();
                nChannelIndex++)
        {
            const Json::Value& jsonChannel = iterTuner->LineUp[nChannelIndex];

            if (jsonChannel["_UID"].asUInt() != channel.iUniqueId)
                continue;

            for (nGuideIndex = 0; nGuideIndex < iterTuner->Guide.size();
                    nGuideIndex++)
                if (iterTuner->Guide[nGuideIndex]["GuideNumber"].asString()
                        == jsonChannel["GuideNumber"].asString())
                    break;

            if (nGuideIndex == iterTuner->Guide.size())
                continue;

            const Json::Value& jsonGuide =
                    iterTuner->Guide[nGuideIndex]["Guide"];
            for (nGuideIndex = 0; nGuideIndex < jsonGuide.size(); nGuideIndex++)
            {
                const Json::Value& jsonGuideItem = jsonGuide[nGuideIndex];
                EPG_TAG tag;

                if ((time_t) jsonGuideItem["EndTime"].asUInt() <= iStart
                        || iEnd < (time_t) jsonGuideItem["StartTime"].asUInt())
                    continue;

                memset(&tag, 0, sizeof(tag));

                String strTitle(jsonGuideItem["Title"].asString());
                String strSynopsis(jsonGuideItem["Synopsis"].asString());
                String strImageURL(jsonGuideItem["ImageURL"].asString());

                tag.iUniqueBroadcastId = jsonGuideItem["_UID"].asUInt();
                tag.strTitle = strTitle.c_str();
                tag.iChannelNumber = channel.iUniqueId;
                tag.startTime = (time_t) jsonGuideItem["StartTime"].asUInt();
                tag.endTime = (time_t) jsonGuideItem["EndTime"].asUInt();
                tag.firstAired =
                        (time_t) jsonGuideItem["OriginalAirdate"].asUInt();
                tag.strPlot = strSynopsis.c_str();
                tag.strIconPath = strImageURL.c_str();
                tag.iSeriesNumber = jsonGuideItem["_SeriesNumber"].asInt();
                tag.iEpisodeNumber = jsonGuideItem["_EpisodeNumber"].asInt();
                tag.iGenreType = jsonGuideItem["_GenreType"].asUInt();

                KODI_LOG(LOG_DEBUG,
                        "PvrGetEPCForChannel "
                        " Channel: %u Sub: %u Start: %u End: %u"
                        " TunerID: %08x - Title %s"
                        ,
                        channel.iChannelNumber,
                        channel.iSubChannelNumber,
                        tag.startTime,
                        tag.endTime,
                        iterTuner->_discover_device.device_id,
                        tag.strTitle
                        );


                g.PVR->TransferEpgEntry(handle, &tag);
            }
        }
    }

    return PVR_ERROR_NO_ERROR;
}

int Lineup::PvrGetChannelGroupsAmount()
{
    return 3;
}

static const char FavoriteChannels[] = "Favorite channels";
static const char HDChannels[]       = "HD channels";
static const char SDChannels[]       = "SD channels";


PVR_ERROR Lineup::PvrGetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
    PVR_CHANNEL_GROUP channelGroup;

    if (bRadio)
        return PVR_ERROR_NO_ERROR;

    memset(&channelGroup, 0, sizeof(channelGroup));

    channelGroup.iPosition = 1;
    PVR_STRCPY(channelGroup.strGroupName, FavoriteChannels);
    g.PVR->TransferChannelGroup(handle, &channelGroup);

    channelGroup.iPosition++;
    PVR_STRCPY(channelGroup.strGroupName, HDChannels);
    g.PVR->TransferChannelGroup(handle, &channelGroup);

    channelGroup.iPosition++;
    PVR_STRCPY(channelGroup.strGroupName, SDChannels);
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

        if (!strcmp(FavoriteChannels, group.strGroupName) && !info._favorite)
            continue;
        if (!strcmp(HDChannels, group.strGroupName) && !info._hd)
            continue;
        if (!strcmp(SDChannels, group.strGroupName) && info._hd)
            continue;

        PVR_CHANNEL_GROUP_MEMBER channelGroupMember = {0};
        PVR_STRCPY(channelGroupMember.strGroupName, group.strGroupName);
        channelGroupMember.iChannelUniqueId = number.ID();
        //channelGroupMember.iChannelUniqueId = number._channel;

        g.PVR->TransferChannelGroupMember(handle, &channelGroupMember);
    }
    return PVR_ERROR_NO_ERROR;
}

}; // namespace

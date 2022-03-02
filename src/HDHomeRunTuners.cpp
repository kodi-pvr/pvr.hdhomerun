/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "HDHomeRunTuners.h"
#include "Utils.h"

#include <kodi/Filesystem.h>
#include <kodi/tools/StringUtils.h>
#include <set>

static const std::string g_strGroupFavoriteChannels("Favorite channels");
static const std::string g_strGroupHDChannels("HD channels");
static const std::string g_strGroupSDChannels("SD channels");

HDHomeRunTuners::~HDHomeRunTuners()
{
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();
}

ADDON_STATUS HDHomeRunTuners::Create()
{
  KODI_LOG(ADDON_LOG_INFO, "%s - Creating the PVR HDHomeRun add-on", __FUNCTION__);

  SettingsType::Get().ReadSettings();
  Update();
  m_running = true;
  m_thread = std::thread([&] { Process(); });

  return ADDON_STATUS_OK;
}

ADDON_STATUS HDHomeRunTuners::SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue)
{
  return SettingsType::Get().SetSetting(settingName, settingValue);
}

void HDHomeRunTuners::Process()
{
  for (;;)
  {
    for (int i = 0; i < 60*60; i++)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      if (!m_running)
        break;
    }

    if (!m_running)
      break;

    if (Update(HDHomeRunTuners::UpdateLineUp | HDHomeRunTuners::UpdateGuide))
      kodi::addon::CInstancePVRClient::TriggerChannelUpdate();
  }
}

unsigned int HDHomeRunTuners::PvrCalculateUniqueId(const std::string& str)
{
  int nHash = (int)std::hash<std::string>()(str);
  return (unsigned int)abs(nHash);
}

PVR_ERROR HDHomeRunTuners::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  capabilities.SetSupportsEPG(true);
  capabilities.SetSupportsTV(true);
  capabilities.SetSupportsRadio(false);
  capabilities.SetSupportsChannelGroups(true);
  capabilities.SetSupportsRecordings(false);
  capabilities.SetSupportsRecordingsUndelete(false);
  capabilities.SetSupportsTimers(false);
  capabilities.SetSupportsRecordingsRename(false);
  capabilities.SetSupportsRecordingsLifetimeChange(false);
  capabilities.SetSupportsDescrambleInfo(false);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetBackendName(std::string& name)
{
  name = "HDHomeRun PVR add-on";
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetBackendVersion(std::string& version)
{
  version = "0.1";
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetConnectionString(std::string& connection)
{
  connection = "connected";
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR HDHomeRunTuners::GetDriveSpace(uint64_t& total, uint64_t& used)
{
  total = 1024 * 1024 * 1024;
  used = 0;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::OnSystemWake()
{
  Update(HDHomeRunTuners::UpdateLineUp | HDHomeRunTuners::UpdateGuide);
  kodi::addon::CInstancePVRClient::TriggerChannelUpdate();
  return PVR_ERROR_NO_ERROR;
}

int HDHomeRunTuners::DiscoverTunersViaHttp(struct hdhomerun_discover_device_t* tuners,
                                           int maxtuners)
{
  int numtuners = 0;

  std::string strJson, jsonReaderError;
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());

  // This API may be removed by the provider in the future without notice; treat an inability
  // to access this URL as if there were no tuners discovered.  Update() will then attempt
  // a normal broadcast discovery and try to find the user's tuner devices that way
  if (GetFileContents("https://api.hdhomerun.com/discover", strJson))
  {
    Json::Value devices;
    if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &devices,
                          &jsonReaderError) &&
        devices.type() == Json::arrayValue)
    {
      for (const auto& device : devices)
      {
        // Tuners are identified by the presence of a DeviceID value in the JSON;
        // this also applies to devices that have both tuners and a storage engine (DVR)
        if (!device["DeviceID"].isNull() && !device["LocalIP"].isNull())
        {
          std::string ipstring = device["LocalIP"].asString();
          if (ipstring.length() > 0)
          {
            uint32_t ip = ntohl(inet_addr(ipstring.c_str()));
            numtuners += hdhomerun_discover_find_devices_custom_v2(
                ip, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, &tuners[numtuners],
                maxtuners - numtuners);
          }
        }

        if (numtuners == maxtuners)
          break;
      }
    }
  }

  return numtuners;
}

bool HDHomeRunTuners::Update(int nMode)
{
  //
  // Discover
  //
  struct hdhomerun_discover_device_t foundDevices[16] = {};
  int nTunerCount = 0;

  // Attempt tuner discovery via HTTP first if the user has it enabled.  The provider may
  // remove the ability for this method to work in the future without notice, so ensure
  // that normal discovery is treated as a fall-through case rather than making these
  // methods mutually exclusive

  if (SettingsType::Get().GetHttpDiscovery())
    nTunerCount = DiscoverTunersViaHttp(foundDevices, 16);

  if (nTunerCount <= 0)
    nTunerCount = hdhomerun_discover_find_devices_custom_v2(
        0, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, foundDevices, 16);

  if (nTunerCount <= 0)
    return false;

  KODI_LOG(ADDON_LOG_DEBUG, "Found %d HDHomeRun tuners", nTunerCount);

  std::string strUrl, strJson, jsonReaderError;
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());
  std::set<std::string> guideNumberSet;
  bool bClearTuners = false;

  AutoLock l(this);

  // if latest discovery found fewer devices than m_Tuners List, clear and start fresh
  if (nMode & UpdateDiscover || nTunerCount < m_Tuners.size())
  {
    bClearTuners = true;
    m_Tuners.clear();
  }

  for (int nTunerIndex = 0; nTunerIndex < nTunerCount; nTunerIndex++)
  {
    Tuner* pTuner = nullptr;

    if (bClearTuners)
    {
      // New device
      Tuner tuner;
      pTuner = &*m_Tuners.insert(m_Tuners.end(), tuner);
    }
    else
    {
      // Find existing device
      for (auto& iter : m_Tuners)
        if (iter.Device.ip_addr == foundDevices[nTunerIndex].ip_addr)
        {
          pTuner = &iter;
          break;
        }

      // Device not found in m_Tuners, Add it.
      if (pTuner == nullptr)
      {
        Tuner tuner;
        pTuner = &*m_Tuners.insert(m_Tuners.end(), tuner);
      }
    }

    //
    // Update device
    //
    pTuner->Device = foundDevices[nTunerIndex];

    //
    // Guide
    //
    if (nMode & UpdateGuide)
    {
      strUrl = kodi::tools::StringUtils::Format("https://my.hdhomerun.com/api/guide.php?DeviceAuth=%s", EncodeURL(pTuner->Device.device_auth).c_str());
      KODI_LOG(ADDON_LOG_DEBUG, "Requesting HDHomeRun guide: %s", strUrl.c_str());

      if (GetFileContents(strUrl.c_str(), strJson))
      {
        if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &pTuner->Guide, &jsonReaderError) &&
          pTuner->Guide.type() == Json::arrayValue)
        {
          for (auto& tunerGuide : pTuner->Guide)
          {
            Json::Value& jsonGuide = tunerGuide["Guide"];

            if (jsonGuide.type() != Json::arrayValue)
              continue;

            for (auto& jsonGuideItem : jsonGuide)
            {
              int iSeriesNumber = EPG_TAG_INVALID_SERIES_EPISODE, iEpisodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;

              jsonGuideItem["_UID"] = PvrCalculateUniqueId(jsonGuideItem["Title"].asString() + jsonGuideItem["EpisodeNumber"].asString() + jsonGuideItem["ImageURL"].asString());

              if (SettingsType::Get().GetMarkNew() &&
                  jsonGuideItem["OriginalAirdate"].asUInt() != 0 &&
                  jsonGuideItem["OriginalAirdate"].asUInt() + 48*60*60 > jsonGuideItem["StartTime"].asUInt())
                jsonGuideItem["Title"] = "*" + jsonGuideItem["Title"].asString();

              unsigned int nGenreType = 0;
              for (const auto& str : jsonGuideItem["Filter"])
              {
                if (str == "News")
                  nGenreType = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
                else if (str == "Comedy")
                  nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
                else if (str == "Kids")
                  nGenreType = EPG_EVENT_CONTENTMASK_CHILDRENYOUTH;
                else if (str == "Movie" || str == "Movies" ||
                         str == "Drama")
                  nGenreType = EPG_EVENT_CONTENTMASK_MOVIEDRAMA;
                else if (str == "Food")
                  nGenreType = EPG_EVENT_CONTENTMASK_LEISUREHOBBIES;
                else if (str == "Talk Show")
                  nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
                else if (str == "Game Show")
                  nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
                else if (str == "Sport" ||
                         str == "Sports")
                  nGenreType = EPG_EVENT_CONTENTMASK_SPORTS;
              }
              jsonGuideItem["_GenreType"] = nGenreType;

              if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "S%dE%d", &iSeriesNumber, &iEpisodeNumber) != 2)
                if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "EP%d-%d", &iSeriesNumber, &iEpisodeNumber) != 2)
                  if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "EP%d", &iEpisodeNumber) == 1)
                    iSeriesNumber = EPG_TAG_INVALID_SERIES_EPISODE;

              jsonGuideItem["_SeriesNumber"] = iSeriesNumber;
              jsonGuideItem["_EpisodeNumber"] = iEpisodeNumber;
            }
          }

          KODI_LOG(ADDON_LOG_DEBUG, "Found %u guide entries", pTuner->Guide.size());
        }
        else
        {
          KODI_LOG(ADDON_LOG_ERROR, "Failed to parse guide", strUrl.c_str());
        }
      }
    }

    //
    // Lineup
    //
    if (nMode & UpdateLineUp)
    {
      strUrl = kodi::tools::StringUtils::Format("%s/lineup.json", pTuner->Device.base_url);

      KODI_LOG(ADDON_LOG_DEBUG, "Requesting HDHomeRun lineup: %s", strUrl.c_str());

      if (GetFileContents(strUrl.c_str(), strJson))
      {
        if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &pTuner->LineUp, &jsonReaderError) &&
          pTuner->LineUp.type() == Json::arrayValue)
        {
          int nChannelNumber = 1;

          for (auto& jsonChannel : pTuner->LineUp)
          {
            bool bHide =
              ((jsonChannel["DRM"].asBool() && SettingsType::Get().GetHideProtected()) ||
               (SettingsType::Get().GetHideDuplicateChannels() && guideNumberSet.find(jsonChannel["GuideNumber"].asString()) != guideNumberSet.end()));

            jsonChannel["_UID"] = PvrCalculateUniqueId(jsonChannel["GuideName"].asString() + jsonChannel["URL"].asString());
            jsonChannel["_ChannelName"] = jsonChannel["GuideName"].asString();

            // Find guide entry
            for (const auto& jsonGuide : pTuner->Guide)
            {
              if (jsonGuide["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
              {
                if (jsonGuide["Affiliate"].asString() != "")
                  jsonChannel["_ChannelName"] = jsonGuide["Affiliate"].asString();
                jsonChannel["_IconPath"] = jsonGuide["ImageURL"].asString();
                break;
              }
            }

            jsonChannel["_Hide"] = bHide;

            int nChannel = 0, nSubChannel = 0;
            if (sscanf(jsonChannel["GuideNumber"].asString().c_str(), "%d.%d", &nChannel, &nSubChannel) != 2)
            {
              nSubChannel = 0;
              if (sscanf(jsonChannel["GuideNumber"].asString().c_str(), "%d", &nChannel) != 1)
                nChannel = nChannelNumber;
            }
            jsonChannel["_ChannelNumber"] = nChannel;
            jsonChannel["_SubChannelNumber"] = nSubChannel;

            if (!bHide)
            {
              guideNumberSet.insert(jsonChannel["GuideNumber"].asString());
              nChannelNumber++;
            }
          }
          KODI_LOG(ADDON_LOG_DEBUG, "Found %u channels", pTuner->LineUp.size());
        }
        else
          KODI_LOG(ADDON_LOG_ERROR, "Failed to parse lineup", strUrl.c_str());
      }
    }
  }
  return true;
}

PVR_ERROR HDHomeRunTuners::GetChannelsAmount(int& amount)
{
  amount = 0;

  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
      if (!jsonChannel["_Hide"].asBool())
        amount++;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  if (radio)
    return PVR_ERROR_NO_ERROR;

  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_Hide"].asBool())
        continue;

      kodi::addon::PVRChannel pvrChannel;

      pvrChannel.SetUniqueId(jsonChannel["_UID"].asUInt());
      pvrChannel.SetChannelNumber(jsonChannel["_ChannelNumber"].asUInt());
      pvrChannel.SetSubChannelNumber(jsonChannel["_SubChannelNumber"].asUInt());
      pvrChannel.SetChannelName(jsonChannel["_ChannelName"].asString());
      pvrChannel.SetIconPath(jsonChannel["_IconPath"].asString());

      results.Add(pvrChannel);
    }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel, std::vector<kodi::addon::PVRStreamProperty>& properties)
{
  std::string strUrl = GetChannelStreamURL(channel);
  if (strUrl.empty())
    return PVR_ERROR_FAILED;

  properties.emplace_back(PVR_STREAM_PROPERTY_STREAMURL, strUrl);
  properties.emplace_back(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
{
  signalStatus.SetAdapterName("PVR HDHomeRun Adapter 1");
  signalStatus.SetAdapterStatus("OK");

  return PVR_ERROR_NO_ERROR;
}

namespace
{

std::string ParseAsW3CDateString(time_t time)
{
  std::tm* tm = std::gmtime(&time);
  char buffer[16];
  std::strftime(buffer, 16, "%Y-%m-%d", tm);

  return buffer;
}

} // unnamed namespace

PVR_ERROR HDHomeRunTuners::GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
  {
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_UID"].asUInt() != channelUid)
        continue;

      for (const auto& iterGuide : iterTuner.Guide)
      {
        if (iterGuide["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
        {
          for (const auto& jsonGuideItem : iterGuide["Guide"])
          {
            if (static_cast<time_t>(jsonGuideItem["EndTime"].asUInt()) <= start || end < static_cast<time_t>(jsonGuideItem["StartTime"].asUInt()))
              continue;

            time_t firstAired = static_cast<time_t>(jsonGuideItem["OriginalAirdate"].asUInt());
            std::string strFirstAired((firstAired > 0) ? ParseAsW3CDateString(firstAired) : "");

            kodi::addon::PVREPGTag tag;

            tag.SetEpisodePartNumber(EPG_TAG_INVALID_SERIES_EPISODE);
            tag.SetUniqueBroadcastId(jsonGuideItem["_UID"].asUInt());
            tag.SetTitle(jsonGuideItem["Title"].asString());
            tag.SetUniqueChannelId(channelUid);
            tag.SetStartTime(static_cast<time_t>(jsonGuideItem["StartTime"].asUInt()));
            tag.SetEndTime(static_cast<time_t>(jsonGuideItem["EndTime"].asUInt()));
            tag.SetFirstAired(strFirstAired);
            tag.SetPlot(jsonGuideItem["Synopsis"].asString());
            tag.SetIconPath(jsonGuideItem["ImageURL"].asString());
            tag.SetSeriesNumber(jsonGuideItem["_SeriesNumber"].asInt());
            tag.SetEpisodeNumber(jsonGuideItem["_EpisodeNumber"].asInt());
            tag.SetGenreType(jsonGuideItem["_GenreType"].asUInt());
            tag.SetEpisodeName(jsonGuideItem["EpisodeTitle"].asString());
            tag.SetSeriesLink(jsonGuideItem["SeriesID"].asString());

            results.Add(tag);
          }
        }
      }
    }
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetChannelGroupsAmount(int& amount)
{
  amount = 3;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
{
  if (radio)
    return PVR_ERROR_NO_ERROR;

  kodi::addon::PVRChannelGroup channelGroup;

  channelGroup.SetPosition(1);
  channelGroup.SetGroupName(g_strGroupFavoriteChannels);
  results.Add(channelGroup);

  channelGroup.SetPosition(2);
  channelGroup.SetGroupName(g_strGroupHDChannels);
  results.Add(channelGroup);

  channelGroup.SetPosition(3);
  channelGroup.SetGroupName(g_strGroupSDChannels);
  results.Add(channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_Hide"].asBool() ||
          (g_strGroupFavoriteChannels == group.GetGroupName() && !jsonChannel["Favorite"].asBool()) ||
          (g_strGroupHDChannels == group.GetGroupName() && !jsonChannel["HD"].asBool()) ||
          (g_strGroupSDChannels == group.GetGroupName() && jsonChannel["HD"].asBool()))
        continue;

      kodi::addon::PVRChannelGroupMember channelGroupMember;

      channelGroupMember.SetGroupName(group.GetGroupName());
      channelGroupMember.SetChannelUniqueId(jsonChannel["_UID"].asUInt());
      channelGroupMember.SetChannelNumber(jsonChannel["_ChannelNumber"].asUInt());
      channelGroupMember.SetSubChannelNumber(jsonChannel["_SubChannelNumber"].asUInt());

      results.Add(channelGroupMember);
    }

  return PVR_ERROR_NO_ERROR;
}

// Function to return stream url from any available device.
// Potential issue: Still possible race condition between test and player start. Without
//        using libhdhomerun and actively managing tuner locks and using *livestream functions
//        this race condition cannot be worked around as i see it.
// ToDo: Potentially implement preferred tuner. At the moment as long as the tuner can
//       show the channel requested, it will test in an unknown* order
//       *unknown = device discovery order, which is not guaranteed to be the same from
//                  startup to startup
std::string HDHomeRunTuners::GetChannelStreamURL(const kodi::addon::PVRChannel& channel)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
  {
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_UID"].asUInt() == channel.GetUniqueId() ||
           (channel.GetChannelNumber() == jsonChannel["_ChannelNumber"].asUInt() &&
            channel.GetSubChannelNumber() == jsonChannel["_SubChannelNumber"].asUInt() &&
            channel.GetChannelName() == jsonChannel["_ChannelName"].asString()))
      {
        kodi::vfs::CFile fileHandle;

        if (fileHandle.CURLCreate(jsonChannel["URL"].asString()))
        {
          fileHandle.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL , "failonerror", "false");
          int returnCode = -1;

          if (fileHandle.CURLOpen(ADDON_READ_NO_CACHE))
          {
            std::string proto = fileHandle.GetPropertyValue(ADDON_FILE_PROPERTY_RESPONSE_PROTOCOL, "");
            std::string::size_type posResponseCode = proto.find(' ');
            if (posResponseCode != std::string::npos)
              returnCode = atoi(proto.c_str() + (posResponseCode + 1));
          }
          fileHandle.Close();

          if (returnCode <= 400)
          {
            return jsonChannel["URL"].asString();
          }
          else if (returnCode == 403)
          {
            KODI_LOG(ADDON_LOG_DEBUG, "Tuner ID: %d URL Unavailable: %s, Error Code: %d, All tuners in use on device",
                        channel.GetUniqueId(), jsonChannel["URL"].asString().c_str(), returnCode);
          }
          else
          {
            // ToDo: Not an oversubscription error, implement a count against specific tuners. If > x non 403 failures, blacklist tuner??
            //       potentially flag date/time of last failure, move tuner to blacklist, retry blacklist device y hours after last failure (24?)
            KODI_LOG(ADDON_LOG_DEBUG, "Tuner ID: %d URL Unavailable: %s, Error Code: %d",
                        channel.GetUniqueId(), jsonChannel["URL"].asString().c_str(), returnCode);
          }
        }
      }
    }
  }

  KODI_LOG(ADDON_LOG_DEBUG, "No Tuners available");
  return "";
}

ADDONCREATOR(HDHomeRunTuners)

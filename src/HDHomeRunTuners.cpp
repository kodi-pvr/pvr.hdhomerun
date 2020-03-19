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

#include "HDHomeRunTuners.h"

#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <kodi/libXBMC_addon.h>
#include <p8-platform/util/StringUtils.h>

#include "client.h"
#include "Utils.h"

static const std::string g_strGroupFavoriteChannels("Favorite channels");
static const std::string g_strGroupHDChannels("HD channels");
static const std::string g_strGroupSDChannels("SD channels");

HDHomeRunTuners::HDHomeRunTuners(PVR_PROPERTIES* pvrProps) : m_epgMaxDays(pvrProps->iEpgMaxDays) {}

unsigned int HDHomeRunTuners::PvrCalculateUniqueId(const std::string& str)
{
  int nHash = (int)std::hash<std::string>()(str);
  return (unsigned int)abs(nHash);
}

bool HDHomeRunTuners::Update(int nMode)
{
  //
  // Discover
  //
  struct hdhomerun_discover_device_t foundDevices[16];
  int nTunerCount = hdhomerun_discover_find_devices_custom_v2(0, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, foundDevices, 16);

  if (nTunerCount <= 0)
    return false;

  KODI_LOG(ADDON::LOG_DEBUG, "Found %d HDHomeRun tuners", nTunerCount);

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
      strUrl = StringUtils::Format("http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s", EncodeURL(pTuner->Device.device_auth).c_str());
      KODI_LOG(ADDON::LOG_DEBUG, "Requesting HDHomeRun guide: %s", strUrl.c_str());

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

              if (g.Settings.bMarkNew &&
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

          KODI_LOG(ADDON::LOG_DEBUG, "Found %u guide entries", pTuner->Guide.size());
        }
        else
        {
          KODI_LOG(ADDON::LOG_ERROR, "Failed to parse guide", strUrl.c_str());
        }
      }
    }

    //
    // Lineup
    //
    if (nMode & UpdateLineUp)
    {
      strUrl = StringUtils::Format("%s/lineup.json", pTuner->Device.base_url);

      KODI_LOG(ADDON::LOG_DEBUG, "Requesting HDHomeRun lineup: %s", strUrl.c_str());

      if (GetFileContents(strUrl.c_str(), strJson))
      {
        if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &pTuner->LineUp, &jsonReaderError) &&
          pTuner->LineUp.type() == Json::arrayValue)
        {
          int nChannelNumber = 1;

          for (auto& jsonChannel : pTuner->LineUp)
          {
            bool bHide =
              ((jsonChannel["DRM"].asBool() && g.Settings.bHideProtected) ||
               (g.Settings.bHideDuplicateChannels && guideNumberSet.find(jsonChannel["GuideNumber"].asString()) != guideNumberSet.end()));

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
          KODI_LOG(ADDON::LOG_DEBUG, "Found %u channels", pTuner->LineUp.size());
        }
        else
          KODI_LOG(ADDON::LOG_ERROR, "Failed to parse lineup", strUrl.c_str());
      }
    }
  }
  return true;
}

int HDHomeRunTuners::PvrGetChannelsAmount()
{
  int nCount = 0;

  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
      if (!jsonChannel["_Hide"].asBool())
        nCount++;

  return nCount;
}

PVR_ERROR HDHomeRunTuners::PvrGetChannels(ADDON_HANDLE handle, bool bRadio)
{
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  AutoLock l(this);

  m_epg.Clear();

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_Hide"].asBool())
        continue;

      PVR_CHANNEL pvrChannel = { 0 };

      pvrChannel.iUniqueId = jsonChannel["_UID"].asUInt();
      pvrChannel.iChannelNumber = jsonChannel["_ChannelNumber"].asUInt();
      pvrChannel.iSubChannelNumber = jsonChannel["_SubChannelNumber"].asUInt();
      strncpy(pvrChannel.strChannelName, jsonChannel["_ChannelName"].asString().c_str(),
              sizeof(pvrChannel.strChannelName) - 1);
      pvrChannel.strChannelName[sizeof(pvrChannel.strChannelName) - 1] = '\0';
      strncpy(pvrChannel.strIconPath, jsonChannel["_IconPath"].asString().c_str(),
              sizeof(pvrChannel.strIconPath) - 1);
      pvrChannel.strIconPath[sizeof(pvrChannel.strIconPath) - 1] = '\0';

      g.PVR->TransferChannelEntry(handle, &pvrChannel);

      m_epg.AddChannel(jsonChannel["_UID"].asUInt(), jsonChannel["_ChannelName"].asString());
    }

  return PVR_ERROR_NO_ERROR;
}

namespace
{

std::string ParseAsW3CDateString(time_t time)
{
  std::tm* tm = std::localtime(&time);
  char buffer[16];
  std::strftime(buffer, 16, "%Y-%m-%d", tm);

  return buffer;
}

} // unnamed namespace

PVR_ERROR HDHomeRunTuners::PvrGetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
  {
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_UID"].asUInt() != iChannelUid)
        continue;

      auto epgChannel = m_epg.GetEpgChannel(iChannelUid);

      for (const auto& iterGuide : iterTuner.Guide)
      {
        if (iterGuide["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
        {
          for (const auto& jsonGuideItem : iterGuide["Guide"])
          {
            if (static_cast<time_t>(jsonGuideItem["EndTime"].asUInt()) <= iStart || iEnd < static_cast<time_t>(jsonGuideItem["StartTime"].asUInt()))
              continue;

            std::shared_ptr<EpgEntry> entry = std::make_shared<EpgEntry>();
            entry->UpdateFrom(jsonGuideItem, iChannelUid);

            if (epgChannel->EntryIsNewOrChanged(entry))
            {
              epgChannel->AddEntryToMainEPG(entry);

              EPG_TAG broadcast = {0};
              broadcast.iEpisodePartNumber = EPG_TAG_INVALID_SERIES_EPISODE;
              entry->UpdateTo(broadcast);
              g.PVR->TransferEpgEntry(handle, &broadcast);
            }
          }
        }
      }

      m_epg.CleanEPG(iChannelUid);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

int HDHomeRunTuners::PvrGetChannelGroupsAmount()
{
  return 3;
}

PVR_ERROR HDHomeRunTuners::PvrGetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  PVR_CHANNEL_GROUP channelGroup = { 0 };

  channelGroup.iPosition = 1;
  strncpy(channelGroup.strGroupName, g_strGroupFavoriteChannels.c_str(),
          sizeof(channelGroup.strGroupName) - 1);
  channelGroup.strGroupName[sizeof(channelGroup.strGroupName) - 1] = '\0';
  g.PVR->TransferChannelGroup(handle, &channelGroup);

  channelGroup.iPosition++;
  strncpy(channelGroup.strGroupName, g_strGroupHDChannels.c_str(),
          sizeof(channelGroup.strGroupName) - 1);
  channelGroup.strGroupName[sizeof(channelGroup.strGroupName) - 1] = '\0';
  g.PVR->TransferChannelGroup(handle, &channelGroup);

  channelGroup.iPosition++;
  strncpy(channelGroup.strGroupName, g_strGroupSDChannels.c_str(),
          sizeof(channelGroup.strGroupName) - 1);
  channelGroup.strGroupName[sizeof(channelGroup.strGroupName) - 1] = '\0';
  g.PVR->TransferChannelGroup(handle, &channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::PvrGetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_Hide"].asBool() ||
          (strcmp(g_strGroupFavoriteChannels.c_str(), group.strGroupName) == 0 && !jsonChannel["Favorite"].asBool()) ||
          (strcmp(g_strGroupHDChannels.c_str(), group.strGroupName) == 0 && !jsonChannel["HD"].asBool()) ||
          (strcmp(g_strGroupSDChannels.c_str(), group.strGroupName) == 0 && jsonChannel["HD"].asBool()))
        continue;

      PVR_CHANNEL_GROUP_MEMBER channelGroupMember = { 0 };

      strncpy(channelGroupMember.strGroupName, group.strGroupName,
              sizeof(channelGroupMember.strGroupName) - 1);
      channelGroupMember.strGroupName[sizeof(channelGroupMember.strGroupName) - 1] = '\0';
      channelGroupMember.iChannelUniqueId = jsonChannel["_UID"].asUInt();
      channelGroupMember.iChannelNumber = jsonChannel["_ChannelNumber"].asUInt();
      channelGroupMember.iSubChannelNumber = jsonChannel["_SubChannelNumber"].asUInt();

      g.PVR->TransferChannelGroupMember(handle, &channelGroupMember);
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
std::string HDHomeRunTuners::GetChannelStreamURL(const PVR_CHANNEL* channel)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
  {
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_UID"].asUInt() == channel->iUniqueId ||
           (channel->iChannelNumber == jsonChannel["_ChannelNumber"].asUInt() &&
            channel->iSubChannelNumber == jsonChannel["_SubChannelNumber"].asUInt() &&
            strcmp(channel->strChannelName, jsonChannel["_ChannelName"].asString().c_str()) == 0))
      {
        void* fileHandle = g.XBMC->CURLCreate(jsonChannel["URL"].asString().c_str());

        if (fileHandle != nullptr)
        {
          g.XBMC->CURLAddOption(fileHandle, XFILE::CURL_OPTION_PROTOCOL , "failonerror", "false");
          int returnCode = -1;

          if (g.XBMC->CURLOpen(fileHandle, XFILE::READ_NO_CACHE))
          {
            std::string proto = g.XBMC->GetFilePropertyValue(fileHandle, XFILE::FILE_PROPERTY_RESPONSE_PROTOCOL, "");
            std::string::size_type posResponseCode = proto.find(' ');
            if (posResponseCode != std::string::npos)
              returnCode = atoi(proto.c_str() + (posResponseCode + 1));
          }
          g.XBMC->CloseFile(fileHandle);

          if (returnCode <= 400)
          {
            return jsonChannel["URL"].asString();
          }
          else if (returnCode == 403)
          {
            KODI_LOG(ADDON::LOG_DEBUG, "Tuner ID: %d URL Unavailable: %s, Error Code: %d, All tuners in use on device", channel->iUniqueId, jsonChannel["URL"].asString().c_str(), returnCode);
          }
          else
          {
            // ToDo: Not an oversubscription error, implement a count against specific tuners. If > x non 403 failures, blacklist tuner??
            //       potentially flag date/time of last failure, move tuner to blacklist, retry blacklist device y hours after last failure (24?)
            KODI_LOG(ADDON::LOG_DEBUG, "Tuner ID: %d URL Unavailable: %s, Error Code: %d", channel->iUniqueId, jsonChannel["URL"].asString().c_str(), returnCode);
          }
        }
      }
    }
  }

  KODI_LOG(ADDON::LOG_DEBUG, "No Tuners available");
  return "";
}

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

#include <p8-platform/util/StringUtils.h>

#include "client.h"
#include "Utils.h"

static const std::string g_strGroupFavoriteChannels("Favorite channels");
static const std::string g_strGroupHDChannels("HD channels");
static const std::string g_strGroupSDChannels("SD channels");

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
  if (nMode & UpdateDiscover || nTunerCount < static_cast<int>(m_Tuners.size()))
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
              int iSeriesNumber = 0, iEpisodeNumber = 0;

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
                    iSeriesNumber = 0;

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
            jsonChannel["_ChannelNumber"] = 0;
            jsonChannel["_SubChannelNumber"] = 0;

            if (!bHide)
            {
              int nChannel = 0, nSubChannel = 0;
              if (sscanf(jsonChannel["GuideNumber"].asString().c_str(), "%d.%d", &nChannel, &nSubChannel) != 2)
              {
                nSubChannel = 0;
                if (sscanf(jsonChannel["GuideNumber"].asString().c_str(), "%d", &nChannel) != 1)
                  nChannel = nChannelNumber;
              }
              jsonChannel["_ChannelNumber"] = nChannel;
              jsonChannel["_SubChannelNumber"] = nSubChannel;
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
    }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::PvrGetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
  {
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_UID"].asUInt() != channel.iUniqueId)
        continue;

      for (const auto& iterGuide : iterTuner.Guide)
      {
        if (iterGuide["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
        {
          for (const auto& jsonGuideItem : iterGuide["Guide"])
          {
            if (static_cast<time_t>(jsonGuideItem["EndTime"].asUInt()) <= iStart || iEnd < static_cast<time_t>(jsonGuideItem["StartTime"].asUInt()))
              continue;

            EPG_TAG tag = { 0 };

            std::string
              strTitle(jsonGuideItem["Title"].asString()),
              strSynopsis(jsonGuideItem["Synopsis"].asString()),
              strEpTitle(jsonGuideItem["EpisodeTitle"].asString()),
              strSeriesID(jsonGuideItem["SeriesID"].asString()),
              strImageURL(jsonGuideItem["ImageURL"].asString());

            tag.iUniqueBroadcastId = jsonGuideItem["_UID"].asUInt();
            tag.strTitle = strTitle.c_str();
            tag.iUniqueChannelId = channel.iUniqueId;
            tag.startTime = static_cast<time_t>(jsonGuideItem["StartTime"].asUInt());
            tag.endTime = static_cast<time_t>(jsonGuideItem["EndTime"].asUInt());
            tag.firstAired = static_cast<time_t>(jsonGuideItem["OriginalAirdate"].asUInt());
            tag.strPlot = strSynopsis.c_str();
            tag.strIconPath = strImageURL.c_str();
            tag.iSeriesNumber = jsonGuideItem["_SeriesNumber"].asInt();
            tag.iEpisodeNumber = jsonGuideItem["_EpisodeNumber"].asInt();
            tag.iGenreType = jsonGuideItem["_GenreType"].asUInt();
            tag.strEpisodeName = strEpTitle.c_str();
            tag.strSeriesLink = strSeriesID.c_str();

            g.PVR->TransferEpgEntry(handle, &tag);
          }
        }
      }
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

std::string HDHomeRunTuners::_GetChannelStreamURL(int iUniqueId)
{
  AutoLock l(this);
  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
      if (jsonChannel["_UID"].asUInt() == iUniqueId)
        return jsonChannel["URL"].asString();

  return "";
}

std::vector<HDHomeRunTuners::Tuner>& HDHomeRunTuners::GetTuners()
{
  return m_Tuners;
}

PVR_ERROR HDHomeRunTuners::GetEPGTagForChannel(EPG_TAG& tag, PVR_CHANNEL& channel, time_t startTime, time_t endTime)
{
  AutoLock l(this);

  for (const auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
    {
      if (jsonChannel["_UID"].asUInt() != channel.iUniqueId)
        continue;

      for (const auto& iterGuide : iterTuner.Guide)
        if (iterGuide["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
          for (const auto& jsonGuideItem : iterGuide["Guide"])
          {
            if (static_cast<time_t>(jsonGuideItem["StartTime"].asUInt()) == startTime &&
                static_cast<time_t>(jsonGuideItem["EndTime"].asUInt()) == endTime)
            {
              std::string
                strTitle(jsonGuideItem["Title"].asString()),
                strSynopsis(jsonGuideItem["Synopsis"].asString()),
                strEpTitle(jsonGuideItem["EpisodeTitle"].asString()),
                strSeriesID(jsonGuideItem["SeriesID"].asString()),
                strImageURL(jsonGuideItem["ImageURL"].asString());

              tag.iUniqueBroadcastId = jsonGuideItem["_UID"].asUInt();
              tag.strTitle = strTitle.c_str();
              tag.iUniqueChannelId = channel.iUniqueId;
              tag.startTime = static_cast<time_t>(jsonGuideItem["StartTime"].asUInt());
              tag.endTime = static_cast<time_t>(jsonGuideItem["EndTime"].asUInt());
              tag.firstAired = static_cast<time_t>(jsonGuideItem["OriginalAirdate"].asUInt());
              tag.strPlot = strSynopsis.c_str();
              tag.strIconPath = strImageURL.c_str();
              tag.iSeriesNumber = jsonGuideItem["_SeriesNumber"].asInt();
              tag.iEpisodeNumber = jsonGuideItem["_EpisodeNumber"].asInt();
              tag.iGenreType = jsonGuideItem["_GenreType"].asUInt();
              tag.strEpisodeName = strEpTitle.c_str();
              tag.strSeriesLink = strSeriesID.c_str();

              return PVR_ERROR_NO_ERROR;
            }
          }
    }

  return PVR_ERROR_FAILED;
}

HDHomeRunTuners::Tuner* HDHomeRunTuners::GetChannelTuners(PVR_CHANNEL& channel)
{
  for (auto& iterTuner : m_Tuners)
    for (const auto& jsonChannel : iterTuner.LineUp)
      if (jsonChannel["_UID"].asUInt() == channel.iUniqueId)
      {
        static Tuner* foundChannelTuner = &iterTuner;
        return foundChannelTuner;
      }

  return nullptr;
}

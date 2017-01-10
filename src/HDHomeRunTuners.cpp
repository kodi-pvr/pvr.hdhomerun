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
#include <set>
#include <functional>

using namespace ADDON;

static const String g_strGroupFavoriteChannels("Favorite channels");
static const String g_strGroupHDChannels("HD channels");
static const String g_strGroupSDChannels("SD channels");

HDHomeRunTuners::HDHomeRunTuners()
{
}

unsigned int HDHomeRunTuners::PvrCalculateUniqueId(const String& str)
{
  int nHash = (int)std::hash<std::string>()(str);
  return (unsigned int)abs(nHash);
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

  //
  // Discover
  //

  nTunerCount = hdhomerun_discover_find_devices_custom_v2(0, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, foundDevices, 16);

  KODI_LOG(LOG_DEBUG, "Found %d HDHomeRun tuners", nTunerCount);

  AutoLock l(this);

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
      for (Tuners::iterator iter = m_Tuners.begin(); iter != m_Tuners.end(); iter++)
        if (iter->Device.ip_addr == foundDevices[nTunerIndex].ip_addr)
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
    pTuner->Device = foundDevices[nTunerIndex];

    //
    // Guide
    //
    
  if (nMode & UpdateGuide)
  {
    strUrl.Format("http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s", EncodeURL(pTuner->Device.device_auth).c_str());

    KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun guide: %s", strUrl.c_str());

    if (GetFileContents(strUrl.c_str(), strJson))
      if (jsonReader.parse(strJson, pTuner->Guide) &&
        pTuner->Guide.type() == Json::arrayValue)
      {
        for (nIndex = 0, nCount = 0; nIndex < pTuner->Guide.size(); nIndex++)
        {
          Json::Value& jsonGuide = pTuner->Guide[nIndex]["Guide"];
          
          if (jsonGuide.type() != Json::arrayValue)
            continue;

          for (Json::Value::ArrayIndex i = 0; i < jsonGuide.size(); i++, nCount++)
          {
            Json::Value& jsonGuideItem = jsonGuide[i];
            int iSeriesNumber = 0, iEpisodeNumber = 0;

            jsonGuideItem["_UID"] = PvrCalculateUniqueId(jsonGuideItem["Title"].asString() + jsonGuideItem["EpisodeNumber"].asString() + jsonGuideItem["ImageURL"].asString());
        
            if (g.Settings.bMarkNew &&
              jsonGuideItem["OriginalAirdate"].asUInt() != 0 && 
                jsonGuideItem["OriginalAirdate"].asUInt() + 48*60*60 > jsonGuideItem["StartTime"].asUInt())
              jsonGuideItem["Title"] = "*" + jsonGuideItem["Title"].asString();
            
            unsigned int nGenreType = 0;
            Json::Value& jsonFilter = jsonGuideItem["Filter"];
            for (Json::Value::ArrayIndex nGenreIndex = 0; nGenreIndex < jsonFilter.size(); nGenreIndex++)
            {
              String str = jsonFilter[nGenreIndex].asString();

              if (str == "News")
                nGenreType = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
              else
              if (str == "Comedy")
                nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
              else
              if (str == "Movie" ||
                str == "Drama")
                nGenreType = EPG_EVENT_CONTENTMASK_MOVIEDRAMA;
              else
              if (str == "Food")
                nGenreType = EPG_EVENT_CONTENTMASK_LEISUREHOBBIES;
              else
              if (str == "Talk Show")
                nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
              else
              if (str == "Game Show")
                nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
              else
              if (str == "Sport" ||
                str == "Sports")
                nGenreType = EPG_EVENT_CONTENTMASK_SPORTS;
            }
            jsonGuideItem["_GenreType"] = nGenreType;

            if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "S%dE%d", &iSeriesNumber, &iEpisodeNumber) != 2)
              if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "EP%d", &iEpisodeNumber) == 1)
                iSeriesNumber = 0;

            jsonGuideItem["_SeriesNumber"] = iSeriesNumber;
            jsonGuideItem["_EpisodeNumber"] = iEpisodeNumber;
          }
        }

        KODI_LOG(LOG_DEBUG, "Found %u guide entries", nCount);
      }
      else
      {
        KODI_LOG(LOG_ERROR, "Failed to parse guide", strUrl.c_str());
      }
    }

  //
  // Lineup
  //

  if (nMode & UpdateLineUp)
  {
    strUrl.Format("%s/lineup.json", pTuner->Device.base_url);

    KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun lineup: %s", strUrl.c_str());

    if (GetFileContents(strUrl.c_str(), strJson))
    {
      if (jsonReader.parse(strJson, pTuner->LineUp) &&
        pTuner->LineUp.type() == Json::arrayValue)
      {
        std::set<String> guideNumberSet;
        int nChannelNumber = 1;

        for (nIndex = 0; nIndex < pTuner->LineUp.size(); nIndex++)
        {
          Json::Value& jsonChannel = pTuner->LineUp[nIndex];
          bool bHide;

          bHide = 
            ((jsonChannel["DRM"].asBool() && g.Settings.bHideProtected) ||
            (g.Settings.bHideDuplicateChannels && guideNumberSet.find(jsonChannel["GuideNumber"].asString()) != guideNumberSet.end()));

          jsonChannel["_UID"] = PvrCalculateUniqueId(jsonChannel["GuideName"].asString() + jsonChannel["URL"].asString());
          jsonChannel["_ChannelName"] = jsonChannel["GuideName"].asString();
                    
          // Find guide entry
          for (nGuideIndex = 0; nGuideIndex < pTuner->Guide.size(); nGuideIndex++)
          {
            const Json::Value& jsonGuide = pTuner->Guide[nGuideIndex];
            if (jsonGuide["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
            {
              if (jsonGuide["Affiliate"].asString() != "")
                jsonChannel["_ChannelName"] = jsonGuide["Affiliate"].asString();
              jsonChannel["_IconPath"] = jsonGuide["ImageURL"].asString();
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

        KODI_LOG(LOG_DEBUG, "Found %u channels", pTuner->LineUp.size());
      }
      else
        KODI_LOG(LOG_ERROR, "Failed to parse lineup", strUrl.c_str());
    }
    }
  }
  
  return true;
}

int HDHomeRunTuners::PvrGetChannelsAmount()
{
  int nCount = 0;

  AutoLock l(this);

  for (Tuners::const_iterator iterTuner = m_Tuners.begin(); iterTuner != m_Tuners.end(); iterTuner++)
    for (Json::Value::ArrayIndex nIndex = 0; nIndex < iterTuner->LineUp.size(); nIndex++)
      if (!iterTuner->LineUp[nIndex]["_Hide"].asBool())
        nCount++;

  return nCount;
}

PVR_ERROR HDHomeRunTuners::PvrGetChannels(ADDON_HANDLE handle, bool bRadio)
{
  PVR_CHANNEL pvrChannel;
  Json::Value::ArrayIndex nIndex;
  
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  AutoLock l(this);
  
  for (Tuners::const_iterator iterTuner = m_Tuners.begin(); iterTuner != m_Tuners.end(); iterTuner++)
    for (nIndex = 0; nIndex < iterTuner->LineUp.size(); nIndex++)
    {
      const Json::Value& jsonChannel = iterTuner->LineUp[nIndex];

      if (jsonChannel["_Hide"].asBool())
        continue;

      memset(&pvrChannel, 0, sizeof(pvrChannel));

      pvrChannel.iUniqueId = jsonChannel["_UID"].asUInt();
      pvrChannel.iChannelNumber = jsonChannel["_ChannelNumber"].asUInt();
      pvrChannel.iSubChannelNumber = jsonChannel["_SubChannelNumber"].asUInt();
      PVR_STRCPY(pvrChannel.strChannelName, jsonChannel["_ChannelName"].asString().c_str());
      PVR_STRCPY(pvrChannel.strStreamURL, jsonChannel["URL"].asString().c_str());
      PVR_STRCPY(pvrChannel.strIconPath, jsonChannel["_IconPath"].asString().c_str());
      
      g.PVR->TransferChannelEntry(handle, &pvrChannel);
    }
  
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::PvrGetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  Json::Value::ArrayIndex nChannelIndex, nGuideIndex;

  AutoLock l(this);

  for (Tuners::const_iterator iterTuner = m_Tuners.begin(); iterTuner != m_Tuners.end(); iterTuner++)
  {
    for (nChannelIndex = 0; nChannelIndex < iterTuner->LineUp.size(); nChannelIndex++)
    {
      const Json::Value& jsonChannel = iterTuner->LineUp[nChannelIndex];

      if (jsonChannel["_UID"].asUInt() != channel.iUniqueId)
        continue;

      for (nGuideIndex = 0; nGuideIndex < iterTuner->Guide.size(); nGuideIndex++)
        if (iterTuner->Guide[nGuideIndex]["GuideNumber"].asString() == jsonChannel["GuideNumber"].asString())
          break;

      if (nGuideIndex == iterTuner->Guide.size())
        continue;

      const Json::Value& jsonGuide = iterTuner->Guide[nGuideIndex]["Guide"];
      for (nGuideIndex = 0; nGuideIndex < jsonGuide.size(); nGuideIndex++)
      {
        const Json::Value& jsonGuideItem = jsonGuide[nGuideIndex];
        EPG_TAG tag;

        if ((time_t)jsonGuideItem["EndTime"].asUInt() <= iStart || iEnd < (time_t)jsonGuideItem["StartTime"].asUInt())
          continue;

        memset(&tag, 0, sizeof(tag));

        String
          strTitle(jsonGuideItem["Title"].asString()),
          strSynopsis(jsonGuideItem["Synopsis"].asString()),
          strImageURL(jsonGuideItem["ImageURL"].asString());

        tag.iUniqueBroadcastId = jsonGuideItem["_UID"].asUInt();
        tag.strTitle = strTitle.c_str();
        tag.iChannelNumber = channel.iUniqueId;
        tag.startTime = (time_t)jsonGuideItem["StartTime"].asUInt();
        tag.endTime = (time_t)jsonGuideItem["EndTime"].asUInt();
        tag.firstAired = (time_t)jsonGuideItem["OriginalAirdate"].asUInt();
        tag.strPlot = strSynopsis.c_str();
        tag.strIconPath = strImageURL.c_str();
        tag.iSeriesNumber = jsonGuideItem["_SeriesNumber"].asInt();
        tag.iEpisodeNumber = jsonGuideItem["_EpisodeNumber"].asInt();
        tag.iGenreType = jsonGuideItem["_GenreType"].asUInt();

        g.PVR->TransferEpgEntry(handle, &tag);
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
  PVR_CHANNEL_GROUP channelGroup;

  if (bRadio)
    return PVR_ERROR_NO_ERROR;
      
  memset(&channelGroup, 0, sizeof(channelGroup));

  channelGroup.iPosition = 1;
  PVR_STRCPY(channelGroup.strGroupName, g_strGroupFavoriteChannels.c_str());
  g.PVR->TransferChannelGroup(handle, &channelGroup);
  
  channelGroup.iPosition++;
  PVR_STRCPY(channelGroup.strGroupName, g_strGroupHDChannels.c_str());
  g.PVR->TransferChannelGroup(handle, &channelGroup);

  channelGroup.iPosition++;
  PVR_STRCPY(channelGroup.strGroupName, g_strGroupSDChannels.c_str());
  g.PVR->TransferChannelGroup(handle, &channelGroup);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::PvrGetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  int nCount = 0;

  AutoLock l(this);

  for (Tuners::const_iterator iterTuner = m_Tuners.begin(); iterTuner != m_Tuners.end(); iterTuner++)
    for (Json::Value::ArrayIndex nChannelIndex = 0; nChannelIndex < iterTuner->LineUp.size(); nChannelIndex++)
    {
      const Json::Value& jsonChannel = iterTuner->LineUp[nChannelIndex];

      if (jsonChannel["_Hide"].asBool() ||
        (strcmp(g_strGroupFavoriteChannels.c_str(), group.strGroupName) == 0 && !jsonChannel["Favorite"].asBool()) ||
        (strcmp(g_strGroupHDChannels.c_str(), group.strGroupName) == 0 && !jsonChannel["HD"].asBool()) ||
        (strcmp(g_strGroupSDChannels.c_str(), group.strGroupName) == 0 && jsonChannel["HD"].asBool()))
        continue;

      PVR_CHANNEL_GROUP_MEMBER channelGroupMember;

      memset(&channelGroupMember, 0, sizeof(channelGroupMember));

      PVR_STRCPY(channelGroupMember.strGroupName, group.strGroupName);
      channelGroupMember.iChannelUniqueId = jsonChannel["_UID"].asUInt();
      channelGroupMember.iChannelNumber = jsonChannel["_ChannelNumber"].asUInt();

      g.PVR->TransferChannelGroupMember(handle, &channelGroupMember);
    }

  return PVR_ERROR_NO_ERROR;
}

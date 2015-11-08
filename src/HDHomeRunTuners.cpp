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

using namespace ADDON; 

HDHomeRunTuners::HDHomeRunTuners()
{
	m_LastUpdate = 0;
}

bool HDHomeRunTuners::Update(int nMode)
{
  struct hdhomerun_discover_device_t foundDevices[16];
  int nCount, nIndex;
  Json::Reader jsonReader;
  Json::Value jsonResponse;

  m_LastUpdate = PLATFORM::GetTimeMs();

  //
  // Discover
  //

  nCount = hdhomerun_discover_find_devices_custom_v2(0, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, foundDevices, 16);

  XBMC->Log(LOG_DEBUG, "Found %d HDHomeRun tuners", nCount);

  AutoLock l(this);

  if (nMode & UpdateDiscover)
	m_Tuners.clear();

  if (nCount <= 0)
	  return false;

  for (nIndex = 0; nIndex < nCount; nIndex++)
  {
	  CStdString strUrl, strJson;
	  Tuner* pTuner = NULL;

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
			  if (iter->Device.ip_addr == foundDevices[nIndex].ip_addr)
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
	  pTuner->Device = foundDevices[nIndex];

	  //
	  // Lineup
	  //

	  if (nMode & UpdateLineUp)
	  {
		  strUrl.Format("%s/lineup.json", pTuner->Device.base_url);

		  XBMC->Log(LOG_DEBUG, "Requesting HDHomeRun lineup: %s", strUrl.c_str());

		  if (GetFileContents(strUrl, strJson) > 0)
			  jsonReader.parse(strJson, pTuner->LineUp);
	  }

	  //
	  // Guide
	  //
	  
	  if (nMode & UpdateGuide)
	  {
		  strUrl.Format("http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s", EncodeURL(pTuner->Device.device_auth).c_str());

		  XBMC->Log(LOG_DEBUG, "Requesting HDHomeRun guide: %s", strUrl.c_str());

		  if (GetFileContents(strUrl, strJson) > 0)
			  jsonReader.parse(strJson, pTuner->Guide);
	  }
  }
  
  return true;
}

unsigned int HDHomeRunTuners::CalculateChannelUniqueId(Json::Value::const_iterator iter)
{
	//return (unsigned int)&*iter;//
	int iId = 0;
	CStdString str((*iter)["GuideName"].asString() + (*iter)["URL"].asString());
	for (const char* p = str.c_str(); *p; p++)
		iId = ((iId << 5) + iId) + *p;
	return (unsigned int)abs(iId);
}

PVR_ERROR HDHomeRunTuners::PvrGetChannels(ADDON_HANDLE handle, bool bRadio)
{
	unsigned int nChannelNumber = 1;
	PVR_CHANNEL pvrChannel;

	if (bRadio)
		return PVR_ERROR_NO_ERROR;

	AutoLock l(this);
	
	for (Tuners::const_iterator iterTuner = m_Tuners.begin(); iterTuner != m_Tuners.end(); iterTuner++)
		for (Json::Value::const_iterator iterChannel = iterTuner->LineUp.begin(); iterChannel != iterTuner->LineUp.end(); iterChannel++, nChannelNumber++)
		{
			memset(&pvrChannel, 0, sizeof(pvrChannel));

			pvrChannel.iUniqueId = CalculateChannelUniqueId(iterChannel);
			pvrChannel.iChannelNumber = nChannelNumber;
			PVR_STRCPY(pvrChannel.strChannelName, ((*iterChannel)["GuideNumber"].asString() + " " + (*iterChannel)["GuideName"].asString()).c_str());
			PVR_STRCPY(pvrChannel.strStreamURL, (*iterChannel)["URL"].asString().c_str());
			
			// Find guide entry
			for (Json::Value::const_iterator iterGuide = iterTuner->Guide.begin(); iterGuide != iterTuner->Guide.end(); iterGuide++)
				if ((*iterGuide)["GuideNumber"].asString() == (*iterChannel)["GuideNumber"].asString())
				{
					PVR_STRCPY(pvrChannel.strChannelName, ((*iterChannel)["GuideNumber"].asString() + " " + (*iterGuide)["Affiliate"].asString()).c_str());
					PVR_STRCPY(pvrChannel.strIconPath, (*iterGuide)["ImageURL"].asString().c_str());
					break;
				}
			
			PVR->TransferChannelEntry(handle, &pvrChannel);
		}
	
	return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDHomeRunTuners::PvrGetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
	Json::Value::const_iterator iter, iterChannel, iterGuide;
	int nBroadcastId = 1;
	EPG_TAG tag;

	if (PLATFORM::GetTimeMs() - m_LastUpdate >= 30 * 60 * 1000)
		Update(UpdateLineUp | UpdateGuide);

	AutoLock l(this);

	for (Tuners::const_iterator iterTuner = m_Tuners.begin(); iterTuner != m_Tuners.end(); iterTuner++)
		for (iterChannel = iterTuner->LineUp.begin(); iterChannel != iterTuner->LineUp.end(); iterChannel++)
		{
			if (CalculateChannelUniqueId(iterChannel) != channel.iUniqueId)
				continue;

			for (iter = iterTuner->Guide.begin(); iter != iterTuner->Guide.end(); iter++)
				if ((*iter)["GuideNumber"].asString() == (*iterChannel)["GuideNumber"].asString())
					break;

			if (iter == iterTuner->Guide.end())
				continue;

			const Json::Value& jsonGuide = (*iter)["Guide"];
			for (iterGuide = jsonGuide.begin(); iterGuide != jsonGuide.end(); iterGuide++)
			{
				if ((time_t)(*iterGuide)["EndTime"].asUInt() <= iStart || iEnd < (time_t)(*iterGuide)["StartTime"].asUInt())
					continue;

				memset(&tag, 0, sizeof(tag));

				CStdString
					strTitle((*iterGuide)["Title"].asString()),
					strSynopsis((*iterGuide)["Synopsis"].asString()),
					strImageURL((*iterGuide)["ImageURL"].asString()),
					strEpisodeNumber((*iterGuide)["EpisodeNumber"].asString()),
					strGenreDescription((*iterGuide)["Filter"][(unsigned int)0].asString());
					
				tag.iUniqueBroadcastId = nBroadcastId++;
				tag.strTitle = strTitle.c_str();
				tag.iChannelNumber = channel.iUniqueId;
				tag.startTime = (time_t)(*iterGuide)["StartTime"].asUInt();
				tag.endTime = (time_t)(*iterGuide)["EndTime"].asUInt();
				tag.firstAired = (time_t)(*iterGuide)["OriginalAirdate"].asUInt();
				tag.strPlot = strSynopsis.c_str();
				tag.strIconPath = strImageURL.c_str();
				if (sscanf(strEpisodeNumber.c_str(), "S%dE%d", &tag.iSeriesNumber, &tag.iEpisodeNumber) != 2)
					if (sscanf(strEpisodeNumber.c_str(), "EP%d", &tag.iEpisodeNumber) == 1)
						tag.iSeriesNumber = 0;
				tag.iGenreType = EPG_GENRE_USE_STRING;
				tag.strGenreDescription = strGenreDescription.c_str();
					
				PVR->TransferEpgEntry(handle, &tag);
			}
		}

	return PVR_ERROR_NO_ERROR;
}

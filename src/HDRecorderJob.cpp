/*
 *  Copyright (C) 2017 Brent Murphy <bmurphy@bcmcs.net>
 *  https://github.com/fuzzard/pvr.hdhomerun
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

#include "HDRecorderJob.h"

#include <ctime>
#include <map>
#include <memory>
#include <string>

#include <json/json.h>
#include <json/writer.h>

#include "HDRecorderUtils.h"
#include "Utils.h"

static const std::string strJobCacheFile = "special://home/userdata/addon_data/pvr.hdhomerun/pvr.hdhomerun_timers.cache";
static const std::string strRecordingsCacheFile = "special://home/userdata/addon_data/pvr.hdhomerun/pvr.hdhomerun_recordings.cache";

HDRecorderJob::HDRecorderJob ()
{
  loadData();
}

HDRecorderJob::~HDRecorderJob()
{
  storeEntryData(JobCache);
  storeEntryData(RecordingCache);
  m_RecordJobMap.clear();
  m_RecordingsMap.clear();
}

bool HDRecorderJob::loadData()
{
  m_RecordJobMap.clear();
  m_RecordingsMap.clear();
  CACHE_STATUS retStatus;
  KODI_LOG(ADDON::LOG_DEBUG, "Loading Timer Cache");
  retStatus = readCache(JobCache);
  if (retStatus == CACHE_NOT_EXIST)
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Timer Cache Does not exist: %s", strJobCacheFile.c_str());
    if (!createCache(JobCache))
    {
      return false;
    }
  }
  KODI_LOG(ADDON::LOG_DEBUG, "Loading Recorder Cache");
  retStatus = readCache(RecordingCache);
  if (retStatus == CACHE_NOT_EXIST)
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Recorder Cache Does not exist: %s", strRecordingsCacheFile.c_str());
    if (!createCache(RecordingCache))
    {
      return false;
    }
  }
  return true;
}

bool HDRecorderJob::createCache(CACHE_TYPE CacheType)
{
  std::string strFile;

  switch(CacheType)
  {
    case JobCache:
    {
      strFile = strJobCacheFile;
      break;
    }
    case RecordingCache:
    {
      strFile = strRecordingsCacheFile;
      break;
    }
  }

  void* fileHandle = g.XBMC->OpenFileForWrite(strFile.c_str(), true);
  if (fileHandle == nullptr)
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Unable to create Empty cache file: %s, Cache Type: %d", strFile.c_str(), CacheType);
    return false;
  }

  Json::StreamWriterBuilder wbuilder;
  Json::Value root = Json::arrayValue;
  std::string emptyCache = Json::writeString(wbuilder, root);

  int writtenbytes = g.XBMC->WriteFile(fileHandle, emptyCache.c_str(), emptyCache.length());
  g.XBMC->FlushFile(fileHandle);
  g.XBMC->CloseFile(fileHandle);
  if (writtenbytes <= 0)
  {
    KODI_LOG(ADDON::LOG_ERROR, "Empty Cache File Creation failed: %s ", strFile.c_str());
    return false;
  }
  KODI_LOG(ADDON::LOG_DEBUG, "Empty Cache File Created: %s", strFile.c_str());
  return true;
}

HDRecorderJob::CACHE_STATUS HDRecorderJob::readCache(CACHE_TYPE CacheType)
{
  std::string strFile, strJson, jsonReaderError;
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());
  Json::Value jsonCache;

  switch(CacheType)
  {
    case JobCache:
    {
      strFile = strJobCacheFile;
      break;
    }
    case RecordingCache:
    {
      strFile = strRecordingsCacheFile;
      break;
    }
  }

  if (g.XBMC->FileExists(strFile.c_str(), false))
  {
    if (GetFileContents(strFile, strJson))
    {
      if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &jsonCache, &jsonReaderError) &&
          jsonCache.type() == Json::arrayValue)
      {
        switch(CacheType)
        {
          case JobCache:
          {
            for (auto& jsonTimer : jsonCache)
            {
              PVR_REC_JOB_ENTRY jobEntry;
              ParseJsonJob(jsonTimer, jobEntry);
              if (jobEntry.Timer.state == PVR_TIMER_STATE_NEW || jobEntry.Timer.state == PVR_TIMER_STATE_SCHEDULED)
              {
                if (jobEntry.Timer.startTime > time(NULL))
                {
                  jobEntry.Status = PVR_STREAM_NO_STREAM;
                  m_RecordJobMap[jobEntry.Timer.iClientIndex] = jobEntry;
                }
              }
            }
            KODI_LOG(ADDON::LOG_DEBUG, "Timer Cache loaded %d entries", m_RecordJobMap.size());
            return CACHE_LOADED;
          }
          case RecordingCache:
          {
            for (auto& jsonRecording : jsonCache)
            {
              PVR_RECORDING_ENTRY RecordingEntry;
              ParseJsonRecording(jsonRecording, RecordingEntry);
              m_RecordingsMap[RecordingEntry.iRecordingId] = RecordingEntry;
            }
            KODI_LOG(ADDON::LOG_DEBUG, "Recording Cache loaded %d entries", m_RecordingsMap.size());
            return CACHE_LOADED;
          }
        }
      }
      else
      {
        KODI_LOG(ADDON::LOG_ERROR, "Unable to parse Cache: %s", strFile.c_str());
        return CACHE_PARSE_FAILED;
      }
    }
    KODI_LOG(ADDON::LOG_ERROR, "Failed to read Cache: %s", strFile.c_str());
    return CACHE_READ_FAILED;
  } 
  return CACHE_NOT_EXIST;
}

bool HDRecorderJob::storeEntryData (CACHE_TYPE CacheType)
{
  std::string strFile;

  switch(CacheType)
  {
    case JobCache:
    {
      strFile = strJobCacheFile;
      break;
    }
    case RecordingCache:
    {
      strFile = strRecordingsCacheFile;
      break;
    }
  }
  KODI_LOG(ADDON::LOG_DEBUG, "Storing Cache Data: %s", strFile.c_str());

  void* fileHandle = g.XBMC->OpenFileForWrite(strFile.c_str(), true);

  if (fileHandle == nullptr)
  {
    KODI_LOG(ADDON::LOG_ERROR, "Unable to open cache file for write: %s ", strFile.c_str());
    return false;
  }
  Json::StreamWriterBuilder wbuilder;
  std::string outputJsonCache;
  Json::Value::ArrayIndex nIndex = 0;
  switch(CacheType)
  {
    case JobCache:
    {
      Json::Value Timers = Json::arrayValue;
      for (auto& it : m_RecordJobMap)
      {
        Json::Value jsonRecJob;
        CreateJsonJob(jsonRecJob, it.second);
        Timers[nIndex] = jsonRecJob;
        KODI_LOG(ADDON::LOG_DEBUG, "Timer JSON Entry: %s ", Timers[nIndex]["Title"].asString().c_str());
        nIndex++;
      }
      outputJsonCache.append(Json::writeString(wbuilder, Timers));
      break;
    }
    case RecordingCache:
    {
      Json::Value Recordings = Json::arrayValue;
      for (auto& it : m_RecordingsMap)
      {
        Json::Value jsonRecording;
        CreateJsonRecording(jsonRecording, it.second);
        Recordings[nIndex] = jsonRecording;
        nIndex++;
      }
      outputJsonCache.append(Json::writeString(wbuilder, Recordings));
      break;
    }
  }
  KODI_LOG(ADDON::LOG_DEBUG, "Data being written: %s", outputJsonCache.c_str());
  int writtenbytes = g.XBMC->WriteFile(fileHandle, outputJsonCache.c_str(), outputJsonCache.length());
  g.XBMC->FlushFile(fileHandle);
  g.XBMC->CloseFile(fileHandle);
  if (writtenbytes <= 0)
  {
    KODI_LOG(ADDON::LOG_ERROR, "Write to Cache File failed: %s ", strFile.c_str());
    return false;
  }
  return true;
}

std::map<int,PVR_REC_JOB_ENTRY> HDRecorderJob::getEntryData(void)
{
  return m_RecordJobMap;
}

std::map<int,PVR_RECORDING_ENTRY> HDRecorderJob::getEntryData(HDRecorderJob::CACHE_TYPE CacheType)
{
  return m_RecordingsMap;
}

bool HDRecorderJob::addJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry)
{
  if (m_RecordJobMap.find(RecJobEntry.Timer.iClientIndex) == m_RecordJobMap.end())
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Add Timer entry %s", RecJobEntry.Timer.strTitle);
    m_RecordJobMap[RecJobEntry.Timer.iClientIndex] = RecJobEntry;
    storeEntryData(JobCache);
    return true;
  }

  return false;
}

bool HDRecorderJob::getJobEntry(const int ientryIndex, PVR_REC_JOB_ENTRY &entry)
{
  if (m_RecordJobMap.find(ientryIndex) == m_RecordJobMap.end()) 
    return false;

  entry = m_RecordJobMap[ientryIndex];
  return true;
}

bool HDRecorderJob::getJobEntry(const std::string strChannelName, PVR_REC_JOB_ENTRY &entry)
{
  for (const auto& it : m_RecordJobMap)
    if (it.second.strChannelName==strChannelName)
    {
      entry = it.second;
      return true;
    }

  return false;
}

bool HDRecorderJob::updateJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry)
{
  PVR_REC_JOB_ENTRY entry = RecJobEntry;
  if (getJobEntry(RecJobEntry.Timer.iClientIndex ,entry))
  {
    entry = RecJobEntry;

    m_RecordJobMap[RecJobEntry.Timer.iClientIndex] = entry;
    storeEntryData(JobCache);
    KODI_LOG(ADDON::LOG_DEBUG, "Updated Timer entry %s", RecJobEntry.Timer.strTitle);
    return true; 
  }
  return false;
}

bool HDRecorderJob::delJobEntry(const int ientryIndex)
{
  PVR_REC_JOB_ENTRY entry;
  if (HDRecorderJob::getJobEntry(ientryIndex ,entry))
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Deleted job entry %s", entry.Timer.strTitle);
    m_RecordJobMap.erase(ientryIndex);
    storeEntryData(JobCache);
    return true;
  }
  return false;
}

bool HDRecorderJob::addRecEntry(const PVR_RECORDING_ENTRY &entry)
{
  if (m_RecordingsMap.find(entry.iRecordingId) == m_RecordingsMap.end())
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Add Recording entry %s", entry.Recording.strTitle);
    m_RecordingsMap[entry.iRecordingId] = entry;
    storeEntryData(RecordingCache);
    return true;
  }

  return false;
}

bool HDRecorderJob::getRecEntry(const int iRecordingId, PVR_RECORDING_ENTRY &entry)
{
  if (m_RecordingsMap.find(iRecordingId) == m_RecordingsMap.end()) 
    return false;

  entry = m_RecordingsMap[iRecordingId];
  return true;
}

bool HDRecorderJob::getRecEntry(const std::string strRecId, PVR_RECORDING_ENTRY &entry)
{
  for (const auto& it : m_RecordingsMap)
    if (it.second.Recording.strRecordingId == strRecId)
    {
      entry = it.second;
      return true;
    }

  return false;
}

bool HDRecorderJob::delRecEntry(const int iRecordingId)
{
  PVR_RECORDING_ENTRY entry;
  if (HDRecorderJob::getRecEntry(iRecordingId ,entry))
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Deleted Recording entry %s", entry.Recording.strTitle);
    m_RecordingsMap.erase(iRecordingId);
    storeEntryData(RecordingCache);
    return true;
  }
  return false;
}

void HDRecorderJob::updateRecEntry(const PVR_RECORDING_ENTRY &RecJobEntry)
{
  PVR_RECORDING_ENTRY entry = RecJobEntry;
  m_RecordingsMap[RecJobEntry.iRecordingId] = entry;
  storeEntryData(RecordingCache);
  KODI_LOG(ADDON::LOG_DEBUG, "Updated Recording entry %s", RecJobEntry.Recording.strTitle);
}

void HDRecorderJob::ParseJsonRecording(Json::Value& jsonRecording, PVR_RECORDING_ENTRY& RecordingEntry)
{
  RecordingEntry.Status = static_cast<PVR_RECORDING_STATUS>(jsonRecording["Status"].asUInt());
  RecordingEntry.iRecordingId = jsonRecording["iRecordingId"].asUInt();
  RecordingEntry.strFileLocation = jsonRecording["strFileLocation"].asString();
  strncpy(RecordingEntry.Recording.strTitle, jsonRecording["PVR_RECORDING"]["strTitle"].asString().c_str(), sizeof(RecordingEntry.Recording.strTitle) - 1);
  RecordingEntry.Recording.strTitle[sizeof(RecordingEntry.Recording.strTitle) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strRecordingId, jsonRecording["PVR_RECORDING"]["strRecordingId"].asString().c_str(), sizeof(RecordingEntry.Recording.strRecordingId) - 1);
  RecordingEntry.Recording.strRecordingId[sizeof(RecordingEntry.Recording.strRecordingId) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strEpisodeName, jsonRecording["PVR_RECORDING"]["strEpisodeName"].asString().c_str(), sizeof(RecordingEntry.Recording.strEpisodeName) - 1);
  RecordingEntry.Recording.strEpisodeName[sizeof(RecordingEntry.Recording.strEpisodeName) - 1] = '\0';
  RecordingEntry.Recording.iSeriesNumber = jsonRecording["PVR_RECORDING"]["iSeriesNumber"].asUInt();
  RecordingEntry.Recording.iEpisodeNumber = jsonRecording["PVR_RECORDING"]["iEpisodeNumber"].asUInt();
  RecordingEntry.Recording.iYear = jsonRecording["PVR_RECORDING"]["iYear"].asUInt();
  strncpy(RecordingEntry.Recording.strDirectory, jsonRecording["PVR_RECORDING"]["strDirectory"].asString().c_str(), sizeof(RecordingEntry.Recording.strDirectory) - 1);
  RecordingEntry.Recording.strDirectory[sizeof(RecordingEntry.Recording.strDirectory) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strPlotOutline, jsonRecording["PVR_RECORDING"]["strPlotOutline"].asString().c_str(), sizeof(RecordingEntry.Recording.strPlotOutline) - 1);
  RecordingEntry.Recording.strPlotOutline[sizeof(RecordingEntry.Recording.strPlotOutline) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strPlot, jsonRecording["PVR_RECORDING"]["strPlot"].asString().c_str(), sizeof(RecordingEntry.Recording.strPlot) - 1);
  RecordingEntry.Recording.strPlot[sizeof(RecordingEntry.Recording.strPlot) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strChannelName, jsonRecording["PVR_RECORDING"]["strChannelName"].asString().c_str(), sizeof(RecordingEntry.Recording.strChannelName) - 1);
  RecordingEntry.Recording.strChannelName[sizeof(RecordingEntry.Recording.strChannelName) - 1] = '\0';
  RecordingEntry.Recording.recordingTime = static_cast<time_t>(std::stol(jsonRecording["PVR_RECORDING"]["recordingTime"].asString()));
  RecordingEntry.Recording.iDuration = jsonRecording["PVR_RECORDING"]["iDuration"].asUInt();
  RecordingEntry.Recording.iPriority = jsonRecording["PVR_RECORDING"]["iPriority"].asUInt();
  RecordingEntry.Recording.iLifetime = jsonRecording["PVR_RECORDING"]["iLifetime"].asUInt();
  RecordingEntry.Recording.iGenreType = jsonRecording["PVR_RECORDING"]["iGenreType"].asUInt();
  RecordingEntry.Recording.iPlayCount = jsonRecording["PVR_RECORDING"]["iPlayCount"].asUInt();
}

void HDRecorderJob::CreateJsonRecording(Json::Value& jsonRecording, PVR_RECORDING_ENTRY& RecordingEntry)
{
  jsonRecording["Status"] = RecordingEntry.Status;
  jsonRecording["iRecordingId"] = RecordingEntry.iRecordingId;
  jsonRecording["strFileLocation"] = RecordingEntry.strFileLocation;
  jsonRecording["PVR_RECORDING"]["strTitle"] = RecordingEntry.Recording.strTitle;
  jsonRecording["PVR_RECORDING"]["strRecordingId"] = RecordingEntry.Recording.strRecordingId;
  jsonRecording["PVR_RECORDING"]["strEpisodeName"] = RecordingEntry.Recording.strEpisodeName;
  jsonRecording["PVR_RECORDING"]["iSeriesNumber"] = RecordingEntry.Recording.iSeriesNumber;
  jsonRecording["PVR_RECORDING"]["iEpisodeNumber"] = RecordingEntry.Recording.iEpisodeNumber;
  jsonRecording["PVR_RECORDING"]["iYear"] = RecordingEntry.Recording.iYear;
  jsonRecording["PVR_RECORDING"]["strDirectory"] = RecordingEntry.Recording.strDirectory;
  jsonRecording["PVR_RECORDING"]["strPlotOutline"] = RecordingEntry.Recording.strPlotOutline;
  jsonRecording["PVR_RECORDING"]["strPlot"] = RecordingEntry.Recording.strPlot;
  jsonRecording["PVR_RECORDING"]["strChannelName"] = RecordingEntry.Recording.strChannelName;
  jsonRecording["PVR_RECORDING"]["recordingTime"] = std::to_string(RecordingEntry.Recording.recordingTime);
  jsonRecording["PVR_RECORDING"]["iDuration"] = RecordingEntry.Recording.iDuration;
  jsonRecording["PVR_RECORDING"]["iPriority"] = RecordingEntry.Recording.iPriority;
  jsonRecording["PVR_RECORDING"]["iLifetime"] = RecordingEntry.Recording.iLifetime;
  jsonRecording["PVR_RECORDING"]["iGenreType"] = RecordingEntry.Recording.iGenreType;
  jsonRecording["PVR_RECORDING"]["iPlayCount"] = RecordingEntry.Recording.iPlayCount;
}

bool HDRecorderJob::ParseJsonJob(Json::Value& jsonRecJob, PVR_REC_JOB_ENTRY& RecJobEntry)
{
  RecJobEntry.Timer.iClientIndex = jsonRecJob["PVR_TIMER"]["iClientIndex"].asUInt();
  if (RecJobEntry.Timer.iClientIndex == PVR_TIMER_NO_CLIENT_INDEX)
    return false;
  RecJobEntry.Timer.iClientChannelUid = jsonRecJob["PVR_TIMER"]["iClientChannelUid"].asUInt();
  RecJobEntry.strChannelName = jsonRecJob["GuideName"].asString();
  RecJobEntry.strGuideNumber = jsonRecJob["GuideNumber"].asString();
  strncpy(RecJobEntry.Timer.strTitle, jsonRecJob["Title"].asString().c_str(), sizeof(RecJobEntry.Timer.strTitle) - 1);
  RecJobEntry.Timer.strTitle[sizeof(RecJobEntry.Timer.strTitle) - 1] = '\0';
  RecJobEntry.Timer.startTime = static_cast<time_t>(std::stol(jsonRecJob["PVR_TIMER"]["startTime"].asString()));
  RecJobEntry.Timer.endTime  = static_cast<time_t>(std::stol(jsonRecJob["PVR_TIMER"]["endTime"].asString()));
  if (jsonRecJob["PVR_TIMER"]["state"].asInt() < 0 || jsonRecJob["PVR_TIMER"]["state"].asUInt() > 10)
    return false;
  RecJobEntry.Timer.state = static_cast<PVR_TIMER_STATE>(jsonRecJob["PVR_TIMER"]["state"].asUInt());
  RecJobEntry.Timer.iPriority = jsonRecJob["PVR_TIMER"]["iPriority"].asUInt();
  RecJobEntry.Timer.iLifetime = jsonRecJob["PVR_TIMER"]["iLifetime"].asUInt();
  RecJobEntry.Timer.firstDay = static_cast<time_t>(std::stol(jsonRecJob["PVR_TIMER"]["firstDay"].asString()));
  RecJobEntry.Timer.iWeekdays = jsonRecJob["PVR_TIMER"]["iWeekdays"].asUInt();
  RecJobEntry.Timer.iEpgUid = jsonRecJob["PVR_TIMER"]["iEpgUid"].asUInt();
  RecJobEntry.Timer.iMarginStart = jsonRecJob["PVR_TIMER"]["iMarginStart"].asUInt();
  RecJobEntry.Timer.iMarginEnd = jsonRecJob["PVR_TIMER"]["iMarginEnd"].asUInt();
  RecJobEntry.Timer.iGenreType = jsonRecJob["PVR_TIMER"]["iGenreType"].asUInt();
  RecJobEntry.Timer.iGenreSubType = jsonRecJob["PVR_TIMER"]["iGenreSubType"].asUInt();
  return true;
}

void HDRecorderJob::CreateJsonJob(Json::Value& jsonRecJob, PVR_REC_JOB_ENTRY& RecJobEntry)
{
  jsonRecJob["GuideName"] = RecJobEntry.strChannelName;
  jsonRecJob["GuideNumber"] = RecJobEntry.strGuideNumber;
  jsonRecJob["Title"] = RecJobEntry.Timer.strTitle;
  jsonRecJob["PVR_TIMER"]["iClientIndex"] = RecJobEntry.Timer.iClientIndex;
  jsonRecJob["PVR_TIMER"]["iClientChannelUid"] = RecJobEntry.Timer.iClientChannelUid;
  jsonRecJob["PVR_TIMER"]["startTime"] = std::to_string(RecJobEntry.Timer.startTime);
  jsonRecJob["PVR_TIMER"]["endTime"] = std::to_string(RecJobEntry.Timer.endTime);
  jsonRecJob["PVR_TIMER"]["state"] = RecJobEntry.Timer.state;
  jsonRecJob["PVR_TIMER"]["iPriority"] = RecJobEntry.Timer.iPriority;
  jsonRecJob["PVR_TIMER"]["iLifetime"] = RecJobEntry.Timer.iLifetime;
  jsonRecJob["PVR_TIMER"]["firstDay"] = std::to_string(RecJobEntry.Timer.firstDay);
  jsonRecJob["PVR_TIMER"]["iWeekdays"] = RecJobEntry.Timer.iWeekdays;
  jsonRecJob["PVR_TIMER"]["iEpgUid"] = RecJobEntry.Timer.iEpgUid;
  jsonRecJob["PVR_TIMER"]["iMarginStart"] = RecJobEntry.Timer.iMarginStart;
  jsonRecJob["PVR_TIMER"]["iMarginEnd"] = RecJobEntry.Timer.iMarginEnd;
  jsonRecJob["PVR_TIMER"]["iGenreType"] = RecJobEntry.Timer.iGenreType;
  jsonRecJob["PVR_TIMER"]["iGenreSubType"] = RecJobEntry.Timer.iGenreSubType;
}

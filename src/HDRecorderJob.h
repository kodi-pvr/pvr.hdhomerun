#pragma once
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

#include <map>
#include <string>

#include <json/json.h>

#include "HDRecorderUtils.h"

class HDRecorderJob
{
public:
  typedef enum
  {
    JobCache = 1,
    RecordingCache = 2,
  } CACHE_TYPE;

private:
  typedef enum
  {
    CACHE_NOT_EXIST = 0,
    CACHE_LOADED = 1,
    CACHE_PARSE_FAILED = 2,
    CACHE_READ_FAILED = 3,
  } CACHE_STATUS;

public:
    HDRecorderJob(void);
    ~HDRecorderJob(void);
    std::map<int,PVR_REC_JOB_ENTRY> getEntryData(void);
    std::map<int,PVR_RECORDING_ENTRY> getEntryData(CACHE_TYPE CacheType);
    // timers
    bool addJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool getJobEntry(const int ientryIndex, PVR_REC_JOB_ENTRY &entry);
    bool getJobEntry(const std::string strChannelName, PVR_REC_JOB_ENTRY &entry);
    bool updateJobEntry(const PVR_REC_JOB_ENTRY &RecJobEntry);
    bool delJobEntry(const int ientryIndex);
    // recordings
    bool addRecEntry(const PVR_RECORDING_ENTRY &entry);
    bool getRecEntry(const int iRecordingId, PVR_RECORDING_ENTRY &entry);
    bool getRecEntry(const std::string strRecId, PVR_RECORDING_ENTRY &entry);
    void updateRecEntry(const PVR_RECORDING_ENTRY &RecJobEntry);
    bool delRecEntry(const int ientryIndex);

private:
    std::map <int,PVR_REC_JOB_ENTRY> m_RecordJobMap;
    std::map <int,PVR_RECORDING_ENTRY> m_RecordingsMap;
    bool createCache(CACHE_TYPE CacheType);
    CACHE_STATUS readCache(CACHE_TYPE CacheType);
    bool loadData(void);
    bool storeEntryData (CACHE_TYPE CacheType);
    void CreateJsonJob(Json::Value& jsonRecJob, PVR_REC_JOB_ENTRY& RecJobEntry);
    bool ParseJsonJob(Json::Value& jsonRecJob, PVR_REC_JOB_ENTRY& RecJobEntry);
    void CreateJsonRecording(Json::Value& jsonRecording, PVR_RECORDING_ENTRY& RecordingEntry);
    void ParseJsonRecording(Json::Value& jsonRecording, PVR_RECORDING_ENTRY& RecordingEntry);
};

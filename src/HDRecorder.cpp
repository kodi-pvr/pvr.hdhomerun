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

#include "HDRecorder.h"

#include <ctime>
#include <chrono>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include <json/json.h>
#include <p8-platform/util/util.h>
#include <p8-platform/threads/mutex.h>
#include <p8-platform/threads/threads.h>

#include "HDHomeRunTuners.h"
#include "HDRecorderJob.h"
#include "HDRecorderScheduler.h"
#include "HDRecorderThread.h"
#include "HDRecorderUtils.h"
#include "Utils.h"

HDRecorder::HDRecorder()
{
  SetTriggerTimerUpdate(false);
  r_HDRecJob = new HDRecorderJob;
  r_HDRecScheduler = new HDRecorderScheduler(r_HDRecJob);
}

HDRecorder::~HDRecorder()
{
  r_HDRecScheduler->StopThread(true);
  g.RECORDER->setLock();
  std::map<int,PVR_REC_JOB_ENTRY> RecordJobs = r_HDRecJob->getEntryData();
  g.RECORDER->setUnlock();
  for (const auto& rec : RecordJobs)
  {
    if (rec.second.Timer.state == PVR_TIMER_STATE_RECORDING)
    {
      KODI_LOG(ADDON::LOG_DEBUG, "HDRecorder Shutdown - Closing Active Thread");
      PVR_REC_JOB_ENTRY entry = rec.second;
      HDRecorderThread* recThread = reinterpret_cast<HDRecorderThread*>(entry.ThreadPtr);
      g.RECORDER->setLock();
      StopRecording(rec.second);
      g.RECORDER->setUnlock();

      PVR_RECORDING_ENTRY RecordingEntry;
      RecordingEntry.Status = PVR_RECORDING_INCOMPLETE;
      g.RECORDER->CreateRecordingItem(rec.second, RecordingEntry);
      g.RECORDER->setLock();
      r_HDRecJob->updateRecEntry(RecordingEntry);
      g.RECORDER->setUnlock();
      g.RECORDER->SetTriggerRecordingUpdate(true);
      // ToDo: Find a way to wait until thread is shutdown
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));
      if (recThread != nullptr)
      {
        delete recThread;
        recThread = nullptr;
        KODI_LOG(ADDON::LOG_DEBUG, "HDRecorder Shutdown - Thread Deleted");
      }
    }
  }
  delete r_HDRecScheduler;
  r_HDRecScheduler = nullptr;
  KODI_LOG(ADDON::LOG_DEBUG, "HDRecScheduler Shutdown");
  delete r_HDRecJob; 
  r_HDRecJob = nullptr;
  KODI_LOG(ADDON::LOG_DEBUG, "HDRecJob Shutdown");
}

// Mutex Locks
void HDRecorder::setLock(void)
{
  rec_mutex.Lock();
}

void HDRecorder::setUnlock(void)
{
  rec_mutex.Unlock();
}

// Recording
bool HDRecorder::StartRecording (const PVR_REC_JOB_ENTRY &RecJobEntry)
{
  PVR_REC_JOB_ENTRY entry;
  if (r_HDRecJob->getJobEntry(RecJobEntry.Timer.iClientIndex, entry))
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Recjob found: %s", RecJobEntry.Timer.strTitle);
    if (g.RECORDER->GetJobChannel(entry))
    {
      KODI_LOG(ADDON::LOG_DEBUG, "Starting recording %s", RecJobEntry.Timer.strTitle);
      entry.Status = PVR_STREAM_START_RECORDING;
      entry.Timer.state = PVR_TIMER_STATE_RECORDING;
      HDRecorderThread* TPtr = new HDRecorderThread(RecJobEntry.Timer.iClientIndex, r_HDRecJob);
      entry.ThreadPtr = TPtr;
      if (!r_HDRecJob->updateJobEntry(entry))
        KODI_LOG(ADDON::LOG_NOTICE, "updateJobEntry failed");
      return true;
    }
    else
    {
      KODI_LOG(ADDON::LOG_DEBUG, "Getjobchannel failed: %s", RecJobEntry.Timer.strTitle);
      entry.Status = PVR_STREAM_STOPPED;
      entry.Timer.state = PVR_TIMER_STATE_ERROR;
      if (entry.ThreadPtr != nullptr)
      {
        delete reinterpret_cast<HDRecorderThread*>(entry.ThreadPtr);
        entry.ThreadPtr = nullptr;
      }

      r_HDRecJob->updateJobEntry(entry);
    }
  }
  return false;
}

void HDRecorder::StopRecording (const PVR_REC_JOB_ENTRY &RecJobEntry)
{
  PVR_REC_JOB_ENTRY entry = RecJobEntry;
  if (entry.Timer.state!=PVR_TIMER_STATE_NEW)
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Stopping recording %s", entry.Timer.strTitle);
    entry.Status = PVR_STREAM_IS_STOPPING;
    r_HDRecJob->updateJobEntry(entry);
  }
}

// rework completely
// add second argument to populate tuner channel
bool HDRecorder::GetJobChannel(PVR_REC_JOB_ENTRY rec)
{
/*  HDHomeRunTuners::Tuners& tuners = g.Tuners->GetTuners();
  HDHomeRunTuners::Tuner* pTuner;
  int nIndex;

//  tuners = g.Tuners->GetTuners();
  for (HDHomeRunTuners::Tuners::iterator iter = tuners.begin(); iter != tuners.end(); iter++)
  {
    pTuner = &*iter;
    for (nIndex = 0; nIndex < pTuner->LineUp.size(); nIndex++)
    {
      if(iter->LineUp[nIndex]["_UID"].asUInt() == rec.iChannelUid)
        return true;
    }
  }

  return false;
*/
return true;
}

// Timers
PVR_ERROR HDRecorder::GetTimers(ADDON_HANDLE handle)
{
  g.RECORDER->setLock();
  std::map<int,PVR_REC_JOB_ENTRY> RecordJobs = r_HDRecJob->getEntryData();
  for (const auto& rec : RecordJobs)
  {
    if (g.RECORDER->GetJobChannel(rec.second))
    {
      r_HDRecJob->updateJobEntry(rec.second);
      g.PVR->TransferTimerEntry(handle, &rec.second.Timer);
    }
  }
  g.RECORDER->setUnlock();
  return PVR_ERROR_NO_ERROR;
}

int HDRecorder::GetTimersAmount(void)
{
  g.RECORDER->setLock();
  std::map<int,PVR_REC_JOB_ENTRY> RecordJobs = r_HDRecJob->getEntryData();
  g.RECORDER->setUnlock();
  return RecordJobs.size();
}

PVR_ERROR HDRecorder::UpdateTimer(const PVR_TIMER &timer)
{
  PVR_TIMER myTimer = timer;
  PVR_REC_JOB_ENTRY entry;
  g.RECORDER->setLock();
  if (r_HDRecJob->getJobEntry(timer.iClientIndex, entry))
  {
    if (entry.Timer.state == PVR_TIMER_STATE_RECORDING &&
        (entry.Status == PVR_STREAM_IS_RECORDING || entry.Status == PVR_STREAM_START_RECORDING || entry.Status == PVR_STREAM_IS_STOPPING))
    {
      //Job is active
      if (timer.state == PVR_TIMER_STATE_CANCELLED)
      {
        //stop recording
        StopRecording(entry);
        PVR_RECORDING_ENTRY RecordingEntry;
        RecordingEntry.Status = PVR_RECORDING_INCOMPLETE;
        g.RECORDER->CreateRecordingItem(entry, RecordingEntry);
        r_HDRecJob->updateRecEntry(RecordingEntry);
        g.RECORDER->SetTriggerRecordingUpdate(true);
      }
      else
      {
        myTimer.state = entry.Timer.state;
      }
    }
    entry.Timer = myTimer;
    r_HDRecJob->updateJobEntry(entry);
    g.RECORDER->SetTriggerTimerUpdate(true);
    g.RECORDER->setUnlock();
    return PVR_ERROR_NO_ERROR;
  }
  g.RECORDER->setUnlock();
  return PVR_ERROR_FAILED;
}

PVR_ERROR HDRecorder::DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  PVR_REC_JOB_ENTRY entry;
  g.RECORDER->setLock();
  if (r_HDRecJob->getJobEntry(timer.iClientIndex, entry))
  {
    StopRecording(entry);
    g.RECORDER->SetTriggerTimerUpdate(true);
    g.RECORDER->setUnlock();
    return PVR_ERROR_NO_ERROR;
  }
  g.RECORDER->setUnlock();
  return PVR_ERROR_FAILED;
}

PVR_ERROR HDRecorder::AddTimer (const PVR_TIMER &timer)
{
  if (g.Settings.strRecPath.length() == 0)
  {
    KODI_LOG(ADDON::LOG_ERROR, "Recording folder is not set. Please change addon configuration.");
    return PVR_ERROR_FAILED;
  }

  //Bad start time and end time  
  if (timer.startTime >= timer.endTime)
    return PVR_ERROR_FAILED;

  //Set start time for Job
  //Correct start time if needed
  time_t startTime, endTime = timer.endTime;
  if (timer.startTime == 0 || timer.startTime < time(nullptr))
    startTime = time(nullptr);
  else
    time_t startTime = timer.startTime;

  //GetChannel definition
  PVR_CHANNEL channel;
  channel.iUniqueId = timer.iClientChannelUid;

  HDHomeRunTuners::Tuner* currentChannelTuner = g.Tuners->GetChannelTuners(channel);
  if (currentChannelTuner == nullptr)
    return PVR_ERROR_FAILED;

  Json::Value::ArrayIndex nIndex = 0;
  int channelIndex = -1;
  for (const auto& jsonChannel : currentChannelTuner->LineUp)
  {
    if (jsonChannel["_UID"].asUInt() == channel.iUniqueId)
    {
      channelIndex = nIndex;
      break;
    }
    nIndex++;
  }

  if (channelIndex == -1)
    return PVR_ERROR_FAILED;

  g.RECORDER->setLock();
  //Check, if job is already schduled for this time
  std::map<int, PVR_REC_JOB_ENTRY> RecJobs = r_HDRecJob->getEntryData();
  g.RECORDER->setUnlock();

  for (const auto& rec : RecJobs)
    if (rec.second.strChannelName == currentChannelTuner->LineUp[channelIndex]["GuideName"].asString())
      if ((rec.second.Timer.endTime > startTime && rec.second.Timer.startTime < endTime) ||
          (rec.second.Timer.startTime < endTime && rec.second.Timer.endTime > startTime) ||
          (rec.second.Timer.startTime < startTime && rec.second.Timer.endTime > endTime) ||
          (rec.second.Timer.startTime > startTime && rec.second.Timer.endTime < endTime))
      {
        //Similar recording already scheduled
        if ((rec.second.Timer.state == PVR_TIMER_STATE_SCHEDULED || (rec.second.Timer.state == PVR_TIMER_STATE_RECORDING)) &&
            (rec.second.Status == PVR_STREAM_IS_RECORDING || rec.second.Status == PVR_STREAM_START_RECORDING))
        {
          //Send message to refresh timers
          KODI_LOG(ADDON::LOG_NOTICE, "Timer already exists");
          return PVR_ERROR_ALREADY_PRESENT;
        }
      }

  //Prepare new entry
  KODI_LOG(ADDON::LOG_DEBUG, "Creating Timer");
  PVR_REC_JOB_ENTRY recJobEntry;
  recJobEntry.Timer = timer;
  recJobEntry.Timer.iClientIndex = g.RECORDER->GetTimersAmount() + 1;

// ToDo: check through guide and grab name. Possibly set timer name to be of structure
//       showname - Episode Numbering
//       may need a function to create the string, as episode numbering is not guaranteed
//       check for legal characters and strip
//  strncpy(recJobEntry.Timer.strTitle, jsonRecJob["Title"].asString().c_str(), sizeof(RecJobEntry.Timer.strTitle) - 1);
//  recJobEntry.Timer.iClientChannelUid = PVR_CHANNEL_INVALID_UID;
  recJobEntry.Timer.startTime = startTime;
  recJobEntry.Timer.endTime = endTime;
  recJobEntry.strChannelName = currentChannelTuner->LineUp[channelIndex]["GuideName"].asString();
  recJobEntry.strGuideNumber = currentChannelTuner->LineUp[channelIndex]["GuideNumber"].asString();

  g.RECORDER->setLock();
  r_HDRecJob->addJobEntry(recJobEntry);

  if (startTime <= time(nullptr))
  {
    recJobEntry.Status = PVR_STREAM_IS_RECORDING;
    recJobEntry.Timer.state = PVR_TIMER_STATE_RECORDING;
    r_HDRecJob->updateJobEntry(recJobEntry);
    StartRecording(recJobEntry);
    g.RECORDER->SetTriggerTimerUpdate(true);
    KODI_LOG(ADDON::LOG_DEBUG, "Timer started, Recording started");
  }
  else
  {
    recJobEntry.Status = PVR_STREAM_NO_STREAM;
    recJobEntry.Timer.state = PVR_TIMER_STATE_SCHEDULED;
    r_HDRecJob->updateJobEntry(recJobEntry);
    KODI_LOG(ADDON::LOG_DEBUG, "Timer scheduled");
  }
  g.RECORDER->setUnlock();
  g.RECORDER->SetTriggerTimerUpdate(true);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR HDRecorder::GetTimerTypes(PVR_TIMER_TYPE types[], int* size)
{

  unsigned int TIMER_ONCE_MANUAL_ATTRIBS
    = PVR_TIMER_TYPE_IS_MANUAL           |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS   |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME   |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE;

/*  unsigned int TIMER_REPEATING_MANUAL_ATTRIBS
    = PVR_TIMER_TYPE_IS_REPEATING                |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE     |
      PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH    |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS           |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME      |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS           |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN   |
      PVR_TIMER_TYPE_SUPPORTS_PRIORITY           |
      PVR_TIMER_TYPE_SUPPORTS_LIFETIME           |
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS  |
      PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL;
*/
  unsigned int TIMER_ONCE_EPG_ATTRIBS
    = PVR_TIMER_TYPE_SUPPORTS_CHANNELS          |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME        |
      PVR_TIMER_TYPE_SUPPORTS_END_TIME          |
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN  |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE;
/*
  unsigned int TIMER_REPEATING_EPG_ATTRIBS
    = PVR_TIMER_TYPE_IS_REPEATING                |
      PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE     |
      PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH    |
      PVR_TIMER_TYPE_SUPPORTS_CHANNELS           |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME         |
      PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME      |
      PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS           |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN   |
      PVR_TIMER_TYPE_SUPPORTS_PRIORITY           |
      PVR_TIMER_TYPE_SUPPORTS_LIFETIME           |
      PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS  |
      PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL;
*/
  /* Timer types definition. */
  std::vector< std::unique_ptr<TimerType> > timerTypes;

  timerTypes.emplace_back(
    /* One-shot manual (time and channel based) */
    std::unique_ptr<TimerType>(new TimerType(
      /* Type id. */
      TIMER_ONCE_MANUAL,
      /* Attributes. */
      TIMER_ONCE_MANUAL_ATTRIBS,
      /* Let Kodi generate the description. */
      "")));

  timerTypes.emplace_back(
    /* One-shot epg based */
    std::unique_ptr<TimerType>(new TimerType(
      /* Type id. */
      TIMER_ONCE_EPG,
      /* Attributes. */
      TIMER_ONCE_EPG_ATTRIBS,
      /* Let Kodi generate the description. */
      "")));

//  timerTypes.emplace_back(
    /* Repeating manual (time and channel based) - timerec */
//    std::unique_ptr<TimerType>(new TimerType(
      /* Type id. */
//      TIMER_REPEATING_MANUAL,
      /* Attributes. */
//      TIMER_REPEATING_MANUAL_ATTRIBS,
      /* Let Kodi generate the description. */
//      "")));

//  timerTypes.emplace_back(
    /* Repeating epg based - autorec */
//    std::unique_ptr<TimerType>(new TimerType(
      /* Type id. */
//      TIMER_REPEATING_EPG,
      /* Attributes. */
//      TIMER_REPEATING_EPG_ATTRIBS,
      /* Let Kodi generate the description. */
//      "")));

  /* Copy data to target array. */
  int i = 0;
  for (auto it = timerTypes.begin(); it != timerTypes.end(); ++it, ++i)
    types[i] = **it;

  *size = timerTypes.size();

  return PVR_ERROR_NO_ERROR;
}

bool HDRecorder::GetTriggerTimerUpdate(void)
{
  return bTriggerTimerUpdate;
}

void HDRecorder::SetTriggerTimerUpdate(bool bTimerUpdate)
{
  bTriggerTimerUpdate = bTimerUpdate;
}

bool HDRecorder::GetTriggerRecordingUpdate(void)
{
  return bTriggerRecordingUpdate;
}

void HDRecorder::SetTriggerRecordingUpdate(bool bRecordingUpdate)
{
  bTriggerRecordingUpdate = bRecordingUpdate;
}

// Recordings
PVR_ERROR HDRecorder::GetRecordings(ADDON_HANDLE handle)
{
  g.RECORDER->setLock();
  std::map<int,PVR_RECORDING_ENTRY> Recordings = r_HDRecJob->getEntryData(HDRecorderJob::RecordingCache);
  for (const auto& rec : Recordings)
    g.PVR->TransferRecordingEntry(handle, &rec.second.Recording);

  g.RECORDER->setUnlock();
  return PVR_ERROR_NO_ERROR;
}

int HDRecorder::GetRecordingsAmount()
{
  g.RECORDER->setLock();
  std::map<int,PVR_RECORDING_ENTRY> Recordings = r_HDRecJob->getEntryData(HDRecorderJob::RecordingCache);
  g.RECORDER->setUnlock();
  return Recordings.size();
}

PVR_ERROR HDRecorder::DeleteRecording(const PVR_RECORDING& recording)
{
  PVR_RECORDING_ENTRY entry;
  g.RECORDER->setLock();
  if (r_HDRecJob->getRecEntry(recording.strRecordingId, entry))
  {
    DeleteRecordingItem(entry);
    g.RECORDER->SetTriggerRecordingUpdate(true);
    g.RECORDER->setUnlock();
    return PVR_ERROR_NO_ERROR;
  }
  g.RECORDER->setUnlock();
  return PVR_ERROR_FAILED;
}

PVR_ERROR HDRecorder::RenameRecording(const PVR_RECORDING& RecordingEntry)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR HDRecorder::SetRecordingPlayCount(const PVR_RECORDING& RecordingEntry, int count)
{
  PVR_RECORDING_ENTRY entry;
  g.RECORDER->setLock();
  if (r_HDRecJob->getRecEntry(RecordingEntry.strRecordingId, entry))
  {
    entry.Recording.iPlayCount = count;
    r_HDRecJob->updateRecEntry(entry);
    g.RECORDER->SetTriggerRecordingUpdate(true);
    g.RECORDER->setUnlock();
    return PVR_ERROR_NO_ERROR;
  }
  g.RECORDER->setUnlock();
  return PVR_ERROR_FAILED;
}

void HDRecorder::DeleteRecordingItem (const PVR_RECORDING_ENTRY& RecordingEntry)
{
  PVR_RECORDING_ENTRY entry = RecordingEntry;
  KODI_LOG(ADDON::LOG_DEBUG, "Deleting recording %s", entry.Recording.strTitle);
  g.RECORDER->setLock();
  r_HDRecJob->delRecEntry(entry.iRecordingId);
  g.RECORDER->setUnlock();
}

void HDRecorder::CreateRecordingItem (const PVR_REC_JOB_ENTRY& RecJobEntry, PVR_RECORDING_ENTRY& RecordingEntry)
{
  PVR_CHANNEL channel;
  channel.iUniqueId = RecJobEntry.Timer.iClientChannelUid;
  g.RECORDER->setLock();
  RecordingEntry.iRecordingId = GetRecordingsAmount();

  strncpy(RecordingEntry.Recording.strRecordingId, std::to_string(GetRecordingsAmount()).c_str(), sizeof(RecordingEntry.Recording.strRecordingId) - 1);
  RecordingEntry.Recording.strRecordingId[sizeof(RecordingEntry.Recording.strRecordingId) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strTitle, RecJobEntry.Timer.strTitle, sizeof(RecordingEntry.Recording.strTitle) - 1);
  RecordingEntry.Recording.strTitle[sizeof(RecordingEntry.Recording.strTitle) - 1] = '\0';
  strncpy(RecordingEntry.Recording.strChannelName, RecJobEntry.strChannelName.c_str(), sizeof(RecordingEntry.Recording.strChannelName) - 1);
  RecordingEntry.Recording.strChannelName[sizeof(RecordingEntry.Recording.strChannelName) - 1] = '\0';

  RecordingEntry.Recording.iPlayCount = 0;
  RecordingEntry.Recording.recordingTime = RecJobEntry.Timer.startTime;

//  std::string strFileLocation = g.Settings.strRecPath + RecJobEntry.Timer.strTitle + ".mpeg2";
  RecordingEntry.strFileLocation = g.Settings.strRecPath + RecJobEntry.Timer.strTitle + ".mpeg2";
// ToDo:
//  strncpy(RecordingEntry.Recording.strDirectory, jsonRecording["PVR_RECORDING"]["strDirectory"].asString().c_str(), sizeof(RecordingEntry.Recording.strDirectory) - 1);
//  RecordingEntry.Recording.iDuration = jsonRecording["PVR_RECORDING"]["iDuration"].asUInt();
//  RecordingEntry.Recording.iPriority = jsonRecording["PVR_RECORDING"]["iPriority"].asUInt();
//  RecordingEntry.Recording.iLifetime = jsonRecording["PVR_RECORDING"]["iLifetime"].asUInt();

  EPG_TAG tag;
  PVR_ERROR myData = g.Tuners->GetEPGTagForChannel(tag, channel, RecJobEntry.Timer.startTime, RecJobEntry.Timer.endTime);
  if (myData == PVR_ERROR_NO_ERROR)
  {
    strncpy(RecordingEntry.Recording.strEpisodeName, tag.strEpisodeName, sizeof(RecordingEntry.Recording.strEpisodeName) - 1);
    RecordingEntry.Recording.strEpisodeName[sizeof(RecordingEntry.Recording.strEpisodeName) - 1] = '\0';
    strncpy(RecordingEntry.Recording.strPlotOutline, tag.strPlotOutline, sizeof(RecordingEntry.Recording.strPlotOutline) - 1);
    RecordingEntry.Recording.strPlotOutline[sizeof(RecordingEntry.Recording.strPlotOutline) - 1] = '\0';
    strncpy(RecordingEntry.Recording.strPlot, tag.strPlot, sizeof(RecordingEntry.Recording.strPlot) - 1);
    RecordingEntry.Recording.strPlot[sizeof(RecordingEntry.Recording.strPlot) - 1] = '\0';
    strncpy(RecordingEntry.Recording.strIconPath, tag.strIconPath, sizeof(RecordingEntry.Recording.strIconPath) - 1);
    RecordingEntry.Recording.strIconPath[sizeof(RecordingEntry.Recording.strIconPath) - 1] = '\0';
    RecordingEntry.Recording.iSeriesNumber = tag.iSeriesNumber;
    RecordingEntry.Recording.iEpisodeNumber = tag.iEpisodeNumber;
    RecordingEntry.Recording.iGenreType = tag.iGenreType;
  }
  g.RECORDER->setUnlock();
}

std::string HDRecorder::GetRecordingFile (const PVR_RECORDING* RecordingEntry)
{
  PVR_RECORDING_ENTRY entry;
  g.RECORDER->setLock();
  if (r_HDRecJob->getRecEntry(RecordingEntry->strRecordingId, entry))
  {
//    RecordingFile = entry.strFileLocation;
    g.RECORDER->setUnlock();
    return entry.strFileLocation;
  }

  g.RECORDER->setUnlock();
  return "";
}

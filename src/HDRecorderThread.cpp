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

#include "HDRecorderThread.h"

#include <ctime>
#include <algorithm>

#include <p8-platform/threads/threads.h>
#include <p8-platform/util/StringUtils.h>

#include "client.h"
#include "HDHomeRunTuners.h"
#include "HDRecorder.h"
#include "HDRecorderJob.h"
#include "HDRecorderUtils.h"
#include "Utils.h"

HDRecorderThread::HDRecorderThread(int iClientIndex, HDRecorderJob* r_HDRecJob)
{
  t_HDRecJob = r_HDRecJob;
  isRunning = false;
  b_stop = false;
  t_iClientIndex = iClientIndex;
  P8PLATFORM::CThread::CreateThread();
}

HDRecorderThread::~HDRecorderThread(void)
{
  b_stop = true;
  this->P8PLATFORM::CThread::StopThread(0);
  KODI_LOG(ADDON::LOG_DEBUG, "HDRecorderThread Deleted");
}

void HDRecorderThread::StopThread(bool bWait)
{
  b_stop = true;
}

void* HDRecorderThread::Process(void)
{
  PVR_REC_JOB_ENTRY entry;
  g.RECORDER->setLock();
  t_HDRecJob->getJobEntry(t_iClientIndex, entry);
  entry.Status = PVR_STREAM_IS_RECORDING;
  t_HDRecJob->updateJobEntry(entry);
  g.RECORDER->setUnlock();

  isRunning = true;

  time_t duration = entry.Timer.endTime-entry.Timer.startTime;
  t_duration = duration;
  KODI_LOG(ADDON::LOG_DEBUG, "Duration: %d, ClientIndex: %d", duration, t_iClientIndex);
  std::string strUrl = g.Tuners->_GetChannelStreamURL(entry.Timer.iClientChannelUid);
  if (strUrl.empty())
  {
    KODI_LOG(ADDON::LOG_NOTICE, "Cannot find stream with valid channel");
    g.RECORDER->setLock();
    entry.Status = PVR_STREAM_STOPPED;
    entry.Timer.state= PVR_TIMER_STATE_ABORTED;
    t_HDRecJob->updateJobEntry(entry);
    g.RECORDER->setUnlock();
    g.RECORDER->SetTriggerTimerUpdate(true);
    isRunning = false;
    return nullptr;
  }
  strUrl = StringUtils::Format("%s?duration=%s", strUrl.c_str(), std::to_string(t_duration).c_str());
  KODI_LOG(ADDON::LOG_DEBUG, "Try to open stream: %s",  strUrl.c_str());

  // ToDo: set filename to ep info if exists?
  //       This definitely needs to be looked t to create unique filenames. Possibly 
  //       have to use date/time to differentiate rather than other supplied tag info
  std::string fileExtension = ".mpeg2";
  std::string videoFile = g.Settings.strRecPath + MakeLegalFileName(entry.Timer.strTitle + fileExtension);
  KODI_LOG(ADDON::LOG_NOTICE, "File to write: %s ", videoFile.c_str());

  void* outputFileHandle = g.XBMC->OpenFileForWrite(videoFile.c_str(), true);
  if (outputFileHandle == nullptr)
  {
    KODI_LOG(ADDON::LOG_NOTICE, "Unable to open Output File: %s", videoFile.c_str());
    g.RECORDER->setLock();
    entry.Status = PVR_STREAM_STOPPED;
    entry.Timer.state= PVR_TIMER_STATE_ABORTED;
    t_HDRecJob->updateJobEntry(entry);
    g.RECORDER->setUnlock();
    g.RECORDER->SetTriggerTimerUpdate(true);
    isRunning = false;
    return nullptr;
  }
// Need to detect 503 for no available tuner
  void* streamFileHandle = g.XBMC->OpenFile(strUrl.c_str(), 0);
  if (streamFileHandle == nullptr)
  {
    KODI_LOG(ADDON::LOG_NOTICE, "Unable to open Source Stream: %s", strUrl.c_str());
    g.RECORDER->setLock();
    entry.Status = PVR_STREAM_STOPPED;
    entry.Timer.state = PVR_TIMER_STATE_ABORTED;
    t_HDRecJob->updateJobEntry(entry);
    g.RECORDER->setUnlock();
    g.RECORDER->SetTriggerTimerUpdate(true);
    isRunning = false;
    return nullptr;
  }

  char buffer[1024];

  for (;;)
  {
    ssize_t bytesRead = g.XBMC->ReadFile(streamFileHandle, buffer, sizeof(buffer));
    if (bytesRead <= 0)
      break;
    g.XBMC->WriteFile(outputFileHandle, buffer, bytesRead);
    std::fill(std::begin(buffer), std::end(buffer), '\0');
    g.RECORDER->setLock();
    t_HDRecJob->getJobEntry(t_iClientIndex, entry);
    g.RECORDER->setUnlock();
    if (entry.Timer.endTime < time(nullptr) || entry.Status == PVR_STREAM_IS_STOPPING || entry.Status == PVR_STREAM_STOPPED || b_stop)
      break;
  }
  g.XBMC->CloseFile(streamFileHandle);
  g.XBMC->CloseFile(outputFileHandle);
  KODI_LOG(ADDON::LOG_NOTICE, "Recording stopped %s ", entry.Timer.strTitle);
  g.RECORDER->setLock();
  entry.Status = PVR_STREAM_STOPPED;
  entry.Timer.state = PVR_TIMER_STATE_COMPLETED;
  t_HDRecJob->updateJobEntry(entry);

  PVR_RECORDING_ENTRY RecordingEntry;
  RecordingEntry.Status = PVR_RECORDING_COMPLETED;
  g.RECORDER->CreateRecordingItem(entry, RecordingEntry);
  t_HDRecJob->updateRecEntry(RecordingEntry);
  g.RECORDER->setUnlock();

  g.RECORDER->SetTriggerRecordingUpdate(true);
  g.RECORDER->SetTriggerTimerUpdate(true);
  isRunning = false;
  return nullptr;
}

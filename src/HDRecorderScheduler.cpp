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

#include "HDRecorderScheduler.h"

#include <map>
#include <ctime>

#include <p8-platform/threads/threads.h>

#include "client.h"
#include "HDRecorder.h"
#include "HDRecorderJob.h"
#include "Utils.h"

HDRecorderScheduler::HDRecorderScheduler(HDRecorderJob* r_HDRecJob)
{
  t_HDRecJob = r_HDRecJob;
  b_interval = 10;
  tLastCheck = time(nullptr) - b_interval;
  isRunning = false;
  b_stop = false;
  P8PLATFORM::CThread::CreateThread();
}

HDRecorderScheduler::~HDRecorderScheduler(void)
{
  b_stop = true;
  KODI_LOG(ADDON::LOG_NOTICE, "Closing scheduler thread");
  this->P8PLATFORM::CThread::StopThread(0);
}

void HDRecorderScheduler::StopThread(bool bWait)
{
  b_stop = true;
}

void* HDRecorderScheduler::Process(void)
{
  KODI_LOG(ADDON::LOG_NOTICE,"Starting scheduler thread");
  isRunning = true;
  while (true) {
    if (b_stop == true)
    {
      isRunning = false;
      return nullptr;
    }
    time_t now = time(nullptr);
    if (now >= (tLastCheck + b_interval)) {
      g.RECORDER->setLock();
      std::map<int,PVR_REC_JOB_ENTRY> RecJobs = t_HDRecJob->getEntryData();
      for (const auto& rec : RecJobs)
      {
        if (rec.second.Status == PVR_STREAM_STOPPED)
        {
          KODI_LOG(ADDON::LOG_NOTICE, "Try to delete Timer %s", rec.second.Timer.strTitle);
          t_HDRecJob->delJobEntry(rec.first);
          g.RECORDER->SetTriggerTimerUpdate(true);
        }
        else if (rec.second.Timer.endTime < time(nullptr))
        {
          KODI_LOG(ADDON::LOG_NOTICE, "Try to delete Timer %s", rec.second.Timer.strTitle);
          t_HDRecJob->delJobEntry(rec.first);
          g.RECORDER->SetTriggerTimerUpdate(true);
        }
        else if ((rec.second.Timer.startTime - 10) <= time(nullptr) && rec.second.Timer.state == PVR_TIMER_STATE_SCHEDULED)
        {
          KODI_LOG(ADDON::LOG_NOTICE, "Try to start recording %s", rec.second.Timer.strTitle);
          g.RECORDER->StartRecording(rec.second);
          g.RECORDER->SetTriggerTimerUpdate(true);
        }
        else if (rec.second.Timer.endTime <= time(nullptr) && (rec.second.Timer.state == PVR_TIMER_STATE_RECORDING))
        {
          KODI_LOG(ADDON::LOG_NOTICE, "Try to stop recording %s", rec.second.Timer.strTitle);
          PVR_RECORDING_ENTRY RecordingEntry;
          RecordingEntry.Status = PVR_RECORDING_COMPLETED;
          g.RECORDER->StopRecording(rec.second);
          g.RECORDER->CreateRecordingItem(rec.second, RecordingEntry);
          t_HDRecJob->updateRecEntry(RecordingEntry);
          g.RECORDER->SetTriggerTimerUpdate(true);
          g.RECORDER->SetTriggerRecordingUpdate(true);
        }
      }
      g.RECORDER->setUnlock();

      if (g.RECORDER->GetTriggerTimerUpdate())
      {
        g.RECORDER->SetTriggerTimerUpdate(false);
        try {
          g.PVR->TriggerTimerUpdate();
        }catch( std::exception const & e ) {
          //Closing Kodi, TriggerTimerUpdate is not available
          // MSVC throws warning C4101: 'e': unreferenced local variable without
          (void)e;
        }
      }
      if (g.RECORDER->GetTriggerRecordingUpdate())
      {
        g.RECORDER->SetTriggerRecordingUpdate(false);
        KODI_LOG(ADDON::LOG_DEBUG,"Recording Trigger Updating");
        try {
          g.PVR->TriggerRecordingUpdate();
        }catch( std::exception const & e ) {
          //Closing Kodi, TriggerTimerUpdate is not available
          // MSVC throws warning C4101: 'e': unreferenced local variable without
          (void)e;
        }
      }
      tLastCheck = time(nullptr);
    }
  }
  return nullptr;
}

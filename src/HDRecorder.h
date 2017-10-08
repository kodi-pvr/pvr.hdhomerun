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

#include <string>

#include <p8-platform/threads/mutex.h>
#include "xbmc_pvr_types.h"

#include "client.h"
#include "HDRecorderUtils.h"

class HDRecorderJob;
class HDRecorderScheduler;

class HDRecorder
{
  public:
    HDRecorder();
    ~HDRecorder();
    PVR_ERROR AddTimer (const PVR_TIMER &timer);
    PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);
    PVR_ERROR GetTimers(ADDON_HANDLE handle);
    int GetTimersAmount(void);
    PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int* size);
    PVR_ERROR UpdateTimer(const PVR_TIMER &timer);
    PVR_ERROR GetRecordings(ADDON_HANDLE handle);
    int GetRecordingsAmount();
    PVR_ERROR DeleteRecording(const PVR_RECORDING& recording);
    PVR_ERROR RenameRecording(const PVR_RECORDING& recording);
    PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING& recording, int count);
    void setLock (void);
    void setUnlock (void);
    bool GetJobChannel(PVR_REC_JOB_ENTRY rec);
    bool GetTriggerTimerUpdate(void);
    void SetTriggerTimerUpdate(bool bTimerUpdate);
    bool GetTriggerRecordingUpdate(void);
    void SetTriggerRecordingUpdate(bool bRecordingUpdate);
    bool StartRecording (const PVR_REC_JOB_ENTRY &RecJobEntry);
    void StopRecording (const PVR_REC_JOB_ENTRY &RecJobEntry);
    void CreateRecordingItem (const PVR_REC_JOB_ENTRY& RecJobEntry, PVR_RECORDING_ENTRY& RecordingEntry);
    std::string GetRecordingFile (const PVR_RECORDING* RecordingEntry);
  private:
    void DeleteRecordingItem (const PVR_RECORDING_ENTRY &RecordingEntry);
    HDRecorderJob* r_HDRecJob;
    HDRecorderScheduler* r_HDRecScheduler;
    bool bTriggerTimerUpdate;
    bool bTriggerRecordingUpdate;
    P8PLATFORM::CMutex rec_mutex;
};


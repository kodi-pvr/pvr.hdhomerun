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

#include <ctime>
#include <string>

#include <xbmc_pvr_types.h>

#include "Utils.h"

// Timer types
#define TIMER_ONCE_MANUAL             (PVR_TIMER_TYPE_NONE + 1)
#define TIMER_ONCE_EPG                (PVR_TIMER_TYPE_NONE + 2)
#define TIMER_REPEATING_MANUAL        (PVR_TIMER_TYPE_NONE + 3)
#define TIMER_REPEATING_EPG           (PVR_TIMER_TYPE_NONE + 4)

struct TimerType : PVR_TIMER_TYPE
{
  TimerType(unsigned int id,
            unsigned int attributes,
            const std::string &description)
  {
    memset(this, 0, sizeof(PVR_TIMER_TYPE));
    iId                              = id;
    iAttributes                      = attributes;
    strncpy(strDescription, description.c_str(), sizeof(strDescription) - 1);
  }
};

typedef enum
{
  PVR_STREAM_NO_STREAM = 0,
  PVR_STREAM_START_RECORDING = 1,
  PVR_STREAM_IS_RECORDING = 2,
  PVR_STREAM_IS_STOPPING = 3,
  PVR_STREAM_STOPPED = 4
} PVR_STREAM_STATUS;

typedef enum
{
  PVR_RECORDING_FAILED = 0,
  PVR_RECORDING_COMPLETED = 1,
  PVR_RECORDING_INCOMPLETE = 2,
} PVR_RECORDING_STATUS;

struct PVR_REC_JOB_ENTRY
{
  PVR_STREAM_STATUS Status;
  unsigned int iChannelUid; 
  std::string strGuideNumber;
  std::string strChannelName;
  PVR_TIMER Timer;
  void* ThreadPtr;
};

struct PVR_RECORDING_ENTRY
{
  PVR_RECORDING_STATUS Status;
  unsigned int iRecordingId;
  std::string strFileLocation;
  PVR_RECORDING Recording;
};

struct RecordingStruct
{
  unsigned int iJobId;
  unsigned int iChannelUid;
  std::string strChannelName;
  std::string strTitle;
  time_t startTime;
  time_t endTime;
  int state;
};

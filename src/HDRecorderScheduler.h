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

#include <p8-platform/threads/threads.h>

class HDRecorderJob;

class HDRecorderScheduler : P8PLATFORM::CThread
{
  public: 
    HDRecorderScheduler(HDRecorderJob* r_HDRecJob);
    virtual ~HDRecorderScheduler(void);
    virtual void StopThread(bool bWait = true);
    virtual void *Process(void);
  private: 
    bool b_stop;
    bool isRunning;
    time_t tLastCheck;
    int b_interval;
    HDRecorderJob* t_HDRecJob;
};

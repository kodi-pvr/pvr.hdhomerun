/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include "hdhomerun.h"
#include <json/json.h>
#include <p8-platform/threads/mutex.h>

#include "client.h"

class HDHomeRunTuners
{
public:
  enum
  {
    UpdateDiscover = 1,
    UpdateLineUp = 2,
    UpdateGuide = 4
  };

  struct Tuner
  {
    Tuner()
    {
      Device = { 0 };
    }

    hdhomerun_discover_device_t Device;
    Json::Value LineUp;
    Json::Value Guide;
  };

  class AutoLock
  {
  public:
    AutoLock(HDHomeRunTuners* p) : m_p(p) { m_p->Lock(); }
    AutoLock(HDHomeRunTuners& p) : m_p(&p) { m_p->Lock(); }
    ~AutoLock() { m_p->Unlock(); }
  protected:
    HDHomeRunTuners* m_p;
  };

  HDHomeRunTuners() {};
  void Lock() { m_Lock.Lock(); }
  void Unlock() { m_Lock.Unlock(); }

  bool Update(int nMode = UpdateDiscover | UpdateLineUp | UpdateGuide);
  PVR_ERROR PvrGetChannels(ADDON_HANDLE handle, bool bRadio);
  int PvrGetChannelsAmount();
  PVR_ERROR PvrGetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd);
  int PvrGetChannelGroupsAmount(void);
  PVR_ERROR PvrGetChannelGroups(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR PvrGetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  std::string GetChannelStreamURL(const PVR_CHANNEL* channel);

private:
  unsigned int PvrCalculateUniqueId(const std::string& str);
  std::vector<Tuner> m_Tuners;
  P8PLATFORM::CMutex m_Lock;
};

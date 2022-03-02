/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "hdhomerun.h"
#include <json/json.h>
#include <kodi/addon-instance/PVR.h>

class ATTRIBUTE_HIDDEN HDHomeRunTuners
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstancePVRClient
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

  HDHomeRunTuners() = default;
  ~HDHomeRunTuners() override;

  void Lock() { m_Lock.lock(); }
  void Unlock() { m_Lock.unlock(); }

  ADDON_STATUS Create() override;
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue) override;

  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  PVR_ERROR GetConnectionString(std::string& connection) override;
  PVR_ERROR GetDriveSpace(uint64_t& total, uint64_t& used) override;

  PVR_ERROR OnSystemWake() override;

  bool Update(int nMode = UpdateDiscover | UpdateLineUp | UpdateGuide);
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannelStreamProperties(const kodi::addon::PVRChannel& channel, std::vector<kodi::addon::PVRStreamProperty>& properties) override;
  PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) override;
  PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results) override;
  PVR_ERROR GetChannelGroupsAmount(int& amount) override;
  PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
  PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group, kodi::addon::PVRChannelGroupMembersResultSet& results) override;

protected:
  void Process();

private:
  std::string GetChannelStreamURL(const kodi::addon::PVRChannel& channel);

  unsigned int PvrCalculateUniqueId(const std::string& str);

  int DiscoverTunersViaHttp(struct hdhomerun_discover_device_t* tuners, int maxtuners);

  std::vector<Tuner> m_Tuners;
  std::atomic<bool> m_running = {false};
  std::thread m_thread;
  std::mutex m_Lock;
};

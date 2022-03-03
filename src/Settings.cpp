/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

SettingsType& SettingsType::Get()
{
  static SettingsType settings;
  return settings;
}

bool SettingsType::ReadSettings()
{
  bHideProtected = kodi::addon::GetSettingBoolean("hide_protected", true);
  bHideDuplicateChannels = kodi::addon::GetSettingBoolean("hide_duplicate", true);
  bMarkNew = kodi::addon::GetSettingBoolean("mark_new", true);
  bDebug = kodi::addon::GetSettingBoolean("debug", false);
  bHttpDiscovery = kodi::addon::GetSettingBoolean("http_discovery", false);

  return true;
}

ADDON_STATUS SettingsType::SetSetting(const std::string& settingName,
                                      const kodi::addon::CSettingValue& settingValue)
{
  if (settingName == "hide_protected")
  {
    bHideProtected = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "hide_duplicate")
  {
    bHideDuplicateChannels = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "mark_new")
    bMarkNew = settingValue.GetBoolean();
  else if (settingName == "debug")
    bDebug = settingValue.GetBoolean();
  else if (settingName == "http_discovery")
  {
    bHttpDiscovery = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }

  return ADDON_STATUS_OK;
}

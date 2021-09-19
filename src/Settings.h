/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>

class ATTRIBUTE_HIDDEN SettingsType
{
public:
  static SettingsType& Get();

  bool ReadSettings();
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue);

  bool GetHideProtected() const { return bHideProtected; }
  bool GetHideDuplicateChannels() const { return bHideDuplicateChannels; }
  bool GetDebug() const { return bDebug; }
  bool GetMarkNew() const { return bMarkNew; }

private:
  SettingsType() = default;

  bool bHideProtected = true;
  bool bHideDuplicateChannels = true;
  bool bDebug = false;
  bool bMarkNew = false;
};

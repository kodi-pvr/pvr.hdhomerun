/*
 *  Copyright (C) 2015-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2015 Zoltan Csizmadia (zcsizmadia@gmail.com)
 *  Copyright (C) 2011 Pulse-Eight (https://www.pulse-eight.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/libXBMC_addon.h>
#include <kodi/libXBMC_pvr.h>

class HDHomeRunTuners;

struct SettingsType
{
  SettingsType()
  {
    bHideProtected = true;
    bHideDuplicateChannels = true;
    bDebug = false;
    bMarkNew = false;
  }

  bool bHideProtected;
  bool bHideDuplicateChannels;
  bool bDebug;
  bool bMarkNew;
};

struct GlobalsType
{
  GlobalsType()
  {
    currentStatus = ADDON_STATUS_UNKNOWN;
    XBMC = NULL;
    PVR = NULL;
    Tuners = NULL;
  }

  ADDON_STATUS currentStatus;
  ADDON::CHelper_libXBMC_addon* XBMC;
  CHelper_libXBMC_pvr* PVR;

  HDHomeRunTuners* Tuners;

  SettingsType Settings;
};

extern GlobalsType g;

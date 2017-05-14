#pragma once
/*
 *      Copyright (C) 2015 Zoltan Csizmadia <zcsizmadia@gmail.com>
 *      https://github.com/zcsizmadia/pvr.hdhomerun
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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

#include <libXBMC_addon.h>
#include <libXBMC_pvr.h>
#include <p8-platform/util/StdString.h>

namespace PVRHDHomeRun {


typedef CStdString String;

class HDHomeRunTuners;
class Lineup;

struct SettingsType
{
    bool bHideProtected         = true;
    bool bHideDuplicateChannels = true;
    bool bDebug                 = false;
    bool bMarkNew               = false;
    bool bAllowUnknownChannels  = false;
    bool bUseLegacy             = false;
};

struct GlobalsType
{
    bool         bCreated                = false;
    ADDON_STATUS currentStatus           = ADDON_STATUS_UNKNOWN;
    unsigned int iCurrentChannelUniqueId = 0;
    String       strUserPath;
    String       strClientPath;
    ADDON::CHelper_libXBMC_addon* XBMC   = nullptr;
    CHelper_libXBMC_pvr*          PVR    = nullptr;

    HDHomeRunTuners*              Tuners = nullptr;
    Lineup*                       lineup = nullptr;

    SettingsType Settings;
};

extern GlobalsType g;

};

#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://xbmc.org
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
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "EpgChannel.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <kodi/libXBMC_pvr.h>

static const int DEFAULT_EPG_MAX_DAYS = 3;

class Epg
{
public:
  Epg(int epgMaxDays);

  void Clear();
  void AddChannel(unsigned int uniqueId, std::string name);
  void CleanEPG(int channelId);
  void SetEPGTimeFrame(int epgMaxDays);
  std::shared_ptr<EpgChannel> GetEpgChannel(int channelId);

private:

  int m_epgMaxDays;
  long m_epgMaxDaysSeconds;

  std::map<unsigned int, std::shared_ptr<EpgChannel>> m_epgChannelsMap;

  mutable std::mutex m_mutex;
};

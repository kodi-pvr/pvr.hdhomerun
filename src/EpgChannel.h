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

#include "EpgEntry.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <kodi/libXBMC_pvr.h>

class EpgEntry;

class EpgChannel
{
public:
  EpgChannel() = default;
  ~EpgChannel() = default;

  int GetUniqueId() const { return m_uniqueId; }
  void SetUniqueId(int value) { m_uniqueId = value; }

  const std::string& GetChannelName() const { return m_channelName; }
  void SetChannelName(const std::string& value) { m_channelName = value; }

  std::map<time_t, std::shared_ptr<EpgEntry>>& GetMainEPG() { return m_mainEPG; }

  bool EntryIsNewOrChanged(const std::shared_ptr<EpgEntry>& entry);
  void AddEntryToMainEPG(std::shared_ptr<EpgEntry>& entry);
  void CleanMainEPG(long epgMaxDaysInSeconds);

private:
  unsigned int m_uniqueId;
  std::string m_channelName;
  std::map<time_t, std::shared_ptr<EpgEntry>> m_mainEPG;
};

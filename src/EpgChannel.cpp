/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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

bool EpgChannel::EntryIsNewOrChanged(const std::shared_ptr<EpgEntry>& entry)
{
  auto epgPair = m_mainEPG.find(entry->GetStartTime());
  if (epgPair != m_mainEPG.end())
  {
    std::shared_ptr<EpgEntry>& existingEntry = epgPair->second;

    return entry && existingEntry && (*entry == *existingEntry);
  }

  return true;
}

void EpgChannel::AddEntryToMainEPG(std::shared_ptr<EpgEntry>& entry)
{
  m_mainEPG[entry->GetStartTime()] = entry;
}

void EpgChannel::CleanMainEPG(long epgMaxDaysInSeconds)
{
  time_t lowerEpgTimeLimit = std::time(nullptr) - epgMaxDaysInSeconds;
  for (auto it = m_mainEPG.cbegin(); it != m_mainEPG.cend();)
  {
    time_t endTime = it->second->GetEndTime();

    if (endTime < lowerEpgTimeLimit)
      it = m_mainEPG.erase(it);
    else
      break; //map keys in order so safe to stop once we find first allowed entry
  }
}

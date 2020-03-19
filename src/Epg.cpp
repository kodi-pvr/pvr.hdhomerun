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

#include "Epg.h"

#include "client.h"

Epg::Epg(int epgMaxDays)
      : m_epgMaxDays(epgMaxDays)
{
  SetEPGTimeFrame(m_epgMaxDays);
}

void Epg::Clear()
{
  m_epgChannelsMap.clear();
}

void Epg::AddChannel(unsigned int uniqueId, std::string name)
{
  std::shared_ptr<EpgChannel> epgChannel = std::make_shared<EpgChannel>();
  epgChannel->SetUniqueId(uniqueId);
  epgChannel->SetChannelName(name);

  m_epgChannelsMap[uniqueId] = epgChannel;
}

void Epg::CleanEPG(int channelId)
{
  GetEpgChannel(channelId)->CleanMainEPG(m_epgMaxDaysSeconds);
}

std::shared_ptr<EpgChannel> Epg::GetEpgChannel(int channelId)
{
  std::shared_ptr<EpgChannel> epgChannel = std::make_shared<EpgChannel>();

  auto epgChannelSearch = m_epgChannelsMap.find(channelId);
  if (epgChannelSearch != m_epgChannelsMap.end())
    epgChannel = epgChannelSearch->second;

  return epgChannel;
}

void Epg::SetEPGTimeFrame(int epgMaxDays)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_epgMaxDays = epgMaxDays;

  if (m_epgMaxDays > 0)
    m_epgMaxDaysSeconds = m_epgMaxDays * 24 * 60 * 60;
  else
    m_epgMaxDaysSeconds = DEFAULT_EPG_MAX_DAYS * 24 * 60 * 60;
}

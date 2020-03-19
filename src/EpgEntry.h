#pragma once
/*
 *      Copyleft (C) 2005-2019 Team XBMC
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

#include <map>
#include <string>

#include <json/json.h>
#include <kodi/libXBMC_pvr.h>

class EpgEntry
{
public:
  const std::string& GetTitle() const { return m_title; }
  void SetTitle(const std::string& value) { m_title = value; }

  unsigned int GetEpgId() const { return m_epgId; }
  void SetEpgId(unsigned int value) { m_epgId = value; }

  int GetChannelId() const { return m_channelId; }
  void SetChannelId(int value) { m_channelId = value; }

  time_t GetStartTime() const { return m_startTime; }
  void SetStartTime(time_t value) { m_startTime = value; }

  time_t GetEndTime() const { return m_endTime; }
  void SetEndTime(time_t value) { m_endTime = value; }

  const std::string& GetFirstAired() const { return m_firstAired; }
  void SetFirstAired(const std::string& value) { m_firstAired = value; }

  const std::string& GetPlot() const { return m_plot; }
  void SetPlot(const std::string& value) { m_plot = value; }

  const std::string& GetIconPath() const { return m_iconPath; }
  void SetIconPath(const std::string& value) { m_iconPath = value; }

  int GetSeriesNumber() const { return m_seriesNumber; }
  void SetSeriesNumber(int value) { m_seriesNumber = value; }

  int GetEpisodeNumber() const { return m_episodeNumber; }
  void SetEpisodeNumber(int value) { m_episodeNumber = value; }

  const std::string& GetEpsiodeTitle() const { return m_episodeTitle; }
  void SetEpsiodeTitle(const std::string& value) { m_episodeTitle = value; }

  const std::string& GetSeriesLink() const { return m_seriesLink; }
  void SetSeriesLink(const std::string& value) { m_seriesLink = value; }

  bool operator==(const EpgEntry& right) const;
  void UpdateTo(EPG_TAG& left) const;
  void UpdateFrom(Json::Value jsonGuideItem, int channelId);

protected:
  std::string m_title;
  unsigned int m_epgId;
  int m_channelId;
  time_t m_startTime;
  time_t m_endTime;
  std::string m_firstAired;
  std::string m_plot;
  std::string m_iconPath;
  int m_seriesNumber;
  int m_episodeNumber;
  unsigned int m_genreType;
  std::string m_episodeTitle;
  std::string m_seriesLink;
};

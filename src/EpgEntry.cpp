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

#include "EpgEntry.h"

#include "EpgChannel.h"

void EpgEntry::UpdateTo(EPG_TAG& left) const
{
  left.iUniqueBroadcastId  = m_epgId;
  left.strTitle            = m_title.c_str();
  left.iUniqueChannelId    = m_channelId;
  left.startTime           = m_startTime;
  left.endTime             = m_endTime;
  left.strPlotOutline      = nullptr;
  left.strPlot             = m_plot.c_str();
  left.strOriginalTitle    = nullptr; // unused
  left.strCast             = nullptr; // unused
  left.strDirector         = nullptr; // unused
  left.strWriter           = nullptr; // unused
  left.iYear               = 0; // unused
  left.strIMDBNumber       = nullptr; // unused
  left.strIconPath         = m_iconPath.c_str();
  left.iGenreType          = m_genreType;
  left.iGenreSubType       = 0; // unused
  left.strGenreDescription = nullptr;
  left.strFirstAired       = m_firstAired.c_str();
  left.iParentalRating     = 0;  // unused
  left.iStarRating         = 0;  // unused
  left.iSeriesNumber       = m_seriesNumber;
  left.iEpisodeNumber      = m_episodeNumber;
  left.strEpisodeName      = m_episodeTitle.c_str();
  left.strSeriesLink      = m_seriesLink.c_str();
  left.iFlags              = EPG_TAG_FLAG_UNDEFINED;
}

bool EpgEntry::operator==(const EpgEntry& right) const
{
  bool isEqual = (m_startTime == right.m_startTime);
  isEqual &= (m_endTime == right.m_endTime);
  isEqual &= (m_channelId == right.m_channelId);
  isEqual &= (m_epgId == right.m_epgId);
  isEqual &= (m_title == right.m_title);
  isEqual &= (m_plot == right.m_plot);
  isEqual &= (m_firstAired == right.m_firstAired);
  isEqual &= (m_genreType == right.m_genreType);
  isEqual &= (m_episodeNumber == right.m_episodeNumber);
  isEqual &= (m_seriesNumber == right.m_seriesNumber);
  isEqual &= (m_iconPath == right.m_iconPath);
  isEqual &= (m_episodeTitle == right.m_episodeTitle);
  isEqual &= (m_seriesLink == right.m_seriesLink);

  return isEqual;
}

namespace
{

std::string ParseAsW3CDateString(time_t time)
{
  std::tm* tm = std::localtime(&time);
  char buffer[16];
  std::strftime(buffer, 16, "%Y-%m-%d", tm);

  return buffer;
}

} // unnamed namespace

void EpgEntry::UpdateFrom(Json::Value jsonGuideItem, int channelId)
{
  //m_epgId = jsonGuideItem["_UID"].asUInt();
  m_epgId = jsonGuideItem["StartTime"].asUInt();
  m_title = jsonGuideItem["Title"].asString();
  m_channelId = channelId;
  m_startTime = static_cast<time_t>(jsonGuideItem["StartTime"].asUInt());
  m_endTime = static_cast<time_t>(jsonGuideItem["EndTime"].asUInt());
  time_t firstAired = static_cast<time_t>(jsonGuideItem["OriginalAirdate"].asUInt());
  m_firstAired = (firstAired > 0) ? ParseAsW3CDateString(firstAired) : "";
  m_plot = jsonGuideItem["Synopsis"].asString();
  m_iconPath = jsonGuideItem["ImageURL"].asString();
  m_seriesNumber = jsonGuideItem["_SeriesNumber"].asInt();
  m_episodeNumber = jsonGuideItem["_EpisodeNumber"].asInt();
  m_genreType = jsonGuideItem["_GenreType"].asUInt();
  m_episodeTitle = jsonGuideItem["EpisodeTitle"].asString();
  m_seriesLink = jsonGuideItem["SeriesID"].asString();
}


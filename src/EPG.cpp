/*
 *      Copyright (C) 2017 Brent Murphy <bmurphy@bcmcs.net>
 *      https://github.com/fuzzard/pvr.hdhomerun
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

#include "EPG.h"

#include <functional>
#include <memory>
#include <string>

#include <json/json.h>

#include "client.h"
#include "xbmc_epg_types.h"

const std::string CEpgBase::SD_GUIDEURL = "http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s";
const std::string CEpgBase::SD_ADVANCEDGUIDE = "AG";

/*
 *  Author: John Cumming
 *  Created: 2016-02-02 Tue 21:21
 *  http://www.jsolutions.co.uk/C++/objectfactory.html
 */

CRegistrar::CRegistrar(const std::string& name, std::function<CEpgBase*(void)> classFactoryFunction)
{
  // register the class factory function
  CEpgFactory::Instance()->RegisterFactoryFunction(name, classFactoryFunction);
}


CEpgFactory* CEpgFactory::Instance()
{
  static CEpgFactory factory;
  return &factory;
}


void CEpgFactory::RegisterFactoryFunction(const std::string& name, std::function<CEpgBase*(void)> classFactoryFunction)
{
  // register the class factory function
  factoryFunctionRegistry[name] = classFactoryFunction;
}


std::shared_ptr<CEpgBase> CEpgFactory::Create(const std::string& name)
{
  CEpgBase* instance = nullptr;

  // find name in the registry and call factory method.
  const auto it = factoryFunctionRegistry.find(name);
  if (it != factoryFunctionRegistry.end())
    instance = it->second();

  // wrap instance in a shared ptr and return
  if (instance != nullptr)
    return std::shared_ptr<CEpgBase>(instance);
  else
    return nullptr;
}

/*
 *      Copyright (C) 2015 Zoltan Csizmadia <zcsizmadia@gmail.com>
 *      https://github.com/zcsizmadia/pvr.hdhomerun
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 */
void CEpgBase::AddGuideInfo(Json::Value& jsonGuide)
{
  for (auto& jsonGuideItem : jsonGuide)
  {
    int iSeriesNumber = 0, iEpisodeNumber = 0;

    jsonGuideItem["_UID"] = g.Tuners->PvrCalculateUniqueId(jsonGuideItem["Title"].asString() + jsonGuideItem["EpisodeNumber"].asString() + jsonGuideItem["ImageURL"].asString());

    if (g.Settings.bMarkNew &&
      jsonGuideItem["OriginalAirdate"].asUInt() != 0 &&
        jsonGuideItem["OriginalAirdate"].asUInt() + 48*60*60 > jsonGuideItem["StartTime"].asUInt())
      jsonGuideItem["Title"] = "*" + jsonGuideItem["Title"].asString();

    unsigned int nGenreType = 0;
    for (const auto& str : jsonGuideItem["Filter"])
    {
      if (str == "News")
        nGenreType = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
      else if (str == "Comedy")
        nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
      else if (str == "Kids")
        nGenreType = EPG_EVENT_CONTENTMASK_CHILDRENYOUTH;
      else if (str == "Movie" || str == "Movies" ||
               str == "Drama")
        nGenreType = EPG_EVENT_CONTENTMASK_MOVIEDRAMA;
      else if (str == "Food")
        nGenreType = EPG_EVENT_CONTENTMASK_LEISUREHOBBIES;
      else if (str == "Talk Show")
        nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
      else if (str == "Game Show")
        nGenreType = EPG_EVENT_CONTENTMASK_SHOW;
      else if (str == "Sport" ||
               str == "Sports")
        nGenreType = EPG_EVENT_CONTENTMASK_SPORTS;
    }
    jsonGuideItem["_GenreType"] = nGenreType;

    if (jsonGuideItem.isMember("EpisodeNumber"))
    {
      if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "S%dE%d", &iSeriesNumber, &iEpisodeNumber) != 2)
        if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "EP%d-%d", &iSeriesNumber, &iEpisodeNumber) != 2)
          if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "EP%d", &iEpisodeNumber) == 1)
            iSeriesNumber = 0;

      // xmltv episode-num format starts at 0 with format x.y.z
      // x = Season numbering starting at 0 (ie season 1 = 0
      // y = Episode
      // z = part number (eg part 1 of 2) disregard this at this stage
      if (sscanf(jsonGuideItem["EpisodeNumber"].asString().c_str(), "%d.%d.", &iSeriesNumber, &iEpisodeNumber) == 2)
      {
        iSeriesNumber++;
        iEpisodeNumber++;
      }
    }
    jsonGuideItem["_SeriesNumber"] = iSeriesNumber;
    jsonGuideItem["_EpisodeNumber"] = iEpisodeNumber;
  }
}

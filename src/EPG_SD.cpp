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

#include "EPG_SD.h"

#include <memory>
#include <string>

#include <json/json.h>
#include <p8-platform/util/StringUtils.h>

#include "HDHomeRunTuners.h"
#include "Utils.h"

REGISTER_CLASS("SD", CEpg_SD);

bool CEpg_SD::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& advancedguide)
{
  const std::string strUrl (StringUtils::Format(CEpgBase::SD_GUIDEURL.c_str(), EncodeURL(pTuner->Device.device_auth).c_str()));

  // No guide Exists, so pull basic guide data to populate epg guide initially
  if (pTuner->Guide.size() < 1 || advancedguide != CEpgBase::SD_ADVANCEDGUIDE)
  {
    return CEpg_SD::UpdateBasicGuide(pTuner, strUrl);
  }
  else if (advancedguide == CEpgBase::SD_ADVANCEDGUIDE)
  {
    CEpg_SD::UpdateAdvancedGuide(pTuner, strUrl);
    return true;
  }
  return false;
}

void CEpg_SD::UpdateAdvancedGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& strUrl)
{
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());

  KODI_LOG(ADDON::LOG_DEBUG, "Requesting HDHomeRun Extended guide: %s", strUrl.c_str());

  if (pTuner->Guide.type() == Json::arrayValue)
  {
    //Loop each channel Guide
    for (auto& iterGuide : pTuner->Guide)
    {
      bool exitExtend = false;
      Json::Value& jsonGuide = iterGuide["Guide"];
      unsigned long long endTime = CEpg_SD::GetEndTime(jsonGuide);

      do 
      {
        std::string strJsonExtended;
        std::string strUrlExtended = StringUtils::Format("%s&Channel=%s&Start=%llu", strUrl.c_str() , iterGuide["GuideNumber"].asString().c_str(), endTime);
        GetFileContents(strUrlExtended.c_str(), strJsonExtended);
        if (strJsonExtended.substr(0,4) == "null" ) 
        {
          exitExtend = true;
        }
        else
        {
          Json::Value tempGuide;
          std::string jsonReaderError;
          if (jsonReader->parse(strJsonExtended.c_str(), strJsonExtended.c_str() + strJsonExtended.size(), &tempGuide, &jsonReaderError) &&
              tempGuide.type() == Json::arrayValue)
          {
            CEpg_SD::InsertGuideData(iterGuide["Guide"], tempGuide);
            endTime = CEpg_SD::GetEndTime(iterGuide["Guide"]);
          }
        }
      } while (!exitExtend);
      KODI_LOG(ADDON::LOG_DEBUG, "Guide Complete for Channel: %s", iterGuide["GuideNumber"].asString().c_str());
      CEpgBase::AddGuideInfo(jsonGuide);
    }
  } 
}

unsigned long long CEpg_SD::GetEndTime(const Json::Value& jsonGuide)
{
  const Json::Value& jsonGuideItem = jsonGuide[jsonGuide.size() - 1];
  if (jsonGuideItem["EndTime"].asUInt() > 0)
    return jsonGuideItem["EndTime"].asUInt();

  return 0;
}

void CEpg_SD::InsertGuideData(Json::Value &Guide, const Json::Value& strInsertdata)
{
  for (Json::Value::ArrayIndex j = 0; j < strInsertdata[0]["Guide"].size(); j++)
    Guide.append(strInsertdata[0]["Guide"][j]);
}

bool CEpg_SD::UpdateBasicGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& strUrl)
{
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());

  KODI_LOG(ADDON::LOG_DEBUG, "Requesting HDHomeRun Basic guide: %s", strUrl.c_str());
  std::string strJson;
  if (GetFileContents(strUrl.c_str(), strJson))
  {
    std::string jsonReaderError;
    if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &pTuner->Guide, &jsonReaderError) &&
        pTuner->Guide.type() == Json::arrayValue)
    {
      for (auto& iterGuide : pTuner->Guide)
      {
        Json::Value& jsonGuide = iterGuide["Guide"];

        if (jsonGuide.type() != Json::arrayValue)
          continue;

        CEpgBase::AddGuideInfo(jsonGuide);
      }
    }
    else
    {
      KODI_LOG(ADDON::LOG_ERROR, "Failed to parse guide", strUrl.c_str());
      return false;
    }
  }
  else
  {
    return false;
  }
  return true;
}

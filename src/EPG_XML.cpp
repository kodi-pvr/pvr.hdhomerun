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

#include "EPG_XML.h"

#include <ctime>
#include <cstring>
#include <memory>
#include <string>

#include <json/json.h>
#include <p8-platform/util/StringUtils.h>
#include <rapidxml/rapidxml.hpp>
#include <zlib.h>

#include "HDHomeRunTuners.h"
#include "Utils.h"

REGISTER_CLASS("XML", CEpg_Xml);

/*
 * This method uses zlib to decompress a gzipped file in memory.
 * Author: Andrew Lim Chong Liang
 * http://windrealm.org
 */
bool CEpg_Xml::GzipInflate(const std::string &compressedBytes, std::string &uncompressedBytes)
{

  #define HANDLE_CALL_ZLIB(status) {   \
    if (status != Z_OK) {       \
      free(uncomp);             \
      return false;             \
    }                           \
  }

  if (compressedBytes.size() == 0)
  {
    uncompressedBytes = compressedBytes;
    return true ;
  }

  uncompressedBytes.clear();

  unsigned full_length = compressedBytes.size();
  unsigned half_length = compressedBytes.size() / 2;

  unsigned uncompLength = full_length;
  char* uncomp = static_cast<char*>(calloc(sizeof(char), uncompLength));

  z_stream strm;
  strm.next_in = (Bytef *) compressedBytes.c_str();
  strm.avail_in = compressedBytes.size() ;
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false ;

  HANDLE_CALL_ZLIB(inflateInit2(&strm, (16+MAX_WBITS)));

  while (!done)
  {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength)
    {
      // Increase size of output buffer
      uncomp = static_cast<char*>(realloc(uncomp, uncompLength + half_length));
      if (uncomp == nullptr)
        return false;
      uncompLength += half_length ;
    }

    strm.next_out = (Bytef *) (uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate (&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK)
    {
      break;
    }
  }

  HANDLE_CALL_ZLIB(inflateEnd (&strm));

  for (size_t i=0; i<strm.total_out; ++i)
  {
    uncompressedBytes += uncomp[ i ];
  }

  free(uncomp);
  return true;
}

/*
 * Next two methods pulled from iptvsimple
 * Author: Anton Fedchin http://github.com/afedchin/xbmc-addon-iptvsimple/
 * Author: Pulse-Eight http://www.pulse-eight.com/
 */

template<class Ch>
inline bool GetNodeValue(const rapidxml::xml_node<Ch> * pRootNode, const char* strTag, std::string& strStringValue)
{
  rapidxml::xml_node<Ch> *pChildNode = pRootNode->first_node(strTag);
  if (pChildNode == nullptr)
    return false;

  strStringValue = pChildNode->value();
  return true;
}

template<class Ch>
inline bool GetAttributeValue(const rapidxml::xml_node<Ch> * pNode, const char* strAttributeName, std::string& strStringValue)
{
  rapidxml::xml_attribute<Ch> *pAttribute = pNode->first_attribute(strAttributeName);
  if (pAttribute == nullptr)
    return false;

  strStringValue = pAttribute->value();
  return true;
}

/*
 *  UpdateGuide()
 *  Accepts a string pointing to an xml or gz compacted xml file. Can be local or remote
 *  Parses xmltv formated file and populates pTuner->Guide with data.
 *
 */
bool CEpg_Xml::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& xmltvlocation)
{
  KODI_LOG(ADDON::LOG_DEBUG, "Starting XMLTV Guide Update: %s", xmltvlocation.c_str());
  if (pTuner->Guide.size() < 1)
    CEpg_Xml::PrepareGuide(pTuner);

  std::string strXMLdata;
  if (GetFileContents(xmltvlocation, strXMLdata))
  {
    // gzip packed
    if (strXMLdata.substr(0,3) == "\x1F\x8B\x08")
    {
      std::string decompressed;
      if (!CEpg_Xml::GzipInflate(strXMLdata, decompressed))
      {
        KODI_LOG(ADDON::LOG_DEBUG, "Invalid EPG file '%s': unable to decompress file.", xmltvlocation.c_str());
        return false;
      }
      strXMLdata = decompressed;
    }
    // data starts as an expected xml file
    // ToDo: Potentially look at using another xml library to run a proper verification
    //       on the file. Not important to this development at this stage however.
    if (!(strXMLdata.substr(0,5) == "<?xml"))
    {
      KODI_LOG(ADDON::LOG_DEBUG, "Invalid EPG file: %s",  xmltvlocation.c_str());
      return false;
    }
    char *xmlbuffer = new char[strXMLdata.size() + 1];
    strcpy(xmlbuffer, strXMLdata.c_str());
    if (!CEpg_Xml::XmlParse(pTuner, xmlbuffer))
      return false;
  }

  if (g.Settings.bXML_icons)
    CEpg_Xml::UseSDIcons(pTuner);

  CEpg_Xml::DuplicateChannelCheck(pTuner);
  for (auto& jsonGuide : pTuner->Guide)
    CEpgBase::AddGuideInfo(jsonGuide["Guide"]);

  KODI_LOG(ADDON::LOG_DEBUG, "Finished XMLTV Guide Update");
  return true;
}

bool CEpg_Xml::XmlParse(HDHomeRunTuners::Tuner *pTuner, char *xmlbuffer)
{
  rapidxml::xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(xmlbuffer);
  }
  catch(rapidxml::parse_error p)
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Unable parse EPG XML: %s", p.what());
    return false;
  }

  rapidxml::xml_node<> *pRootElement = xmlDoc.first_node("tv");
  if (!pRootElement)
  {
    KODI_LOG(ADDON::LOG_DEBUG, "Invalid EPG XML: no <tv> tag found");
    return false;
  }
  KODI_LOG(ADDON::LOG_DEBUG, "Parsing EPG XML: <channel>");
  if (!CEpg_Xml::XmlParseElement(pTuner, pRootElement, "channel"))
    return false;
  KODI_LOG(ADDON::LOG_DEBUG, "Parsing EPG XML: <programme>");
  if (!CEpg_Xml::XmlParseElement(pTuner, pRootElement, "programme"))
    return false;

  return true;
}

bool CEpg_Xml::UseSDIcons(HDHomeRunTuners::Tuner *pTuner)
{
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());

  std::string strUrl = StringUtils::Format(CEpgBase::SD_GUIDEURL.c_str(), EncodeURL(pTuner->Device.device_auth).c_str());
  std::string strJson;
  if (GetFileContents(strUrl.c_str(), strJson))
  {
    std::string jsonReaderError;
    Json::Value jsonIconGuide;
    if (jsonReader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &jsonIconGuide, &jsonReaderError) &&
        jsonIconGuide.type() == Json::arrayValue)
    {
      for (const auto& jsonIconGuideItem : jsonIconGuide)
        for (auto& jsonGuideItem : pTuner->Guide)
          if (strcmp(jsonGuideItem["GuideNumber"].asString().c_str(), jsonIconGuideItem["GuideNumber"].asString().c_str()) == 0)
          {
            jsonGuideItem["ImageURL"] = jsonIconGuideItem["ImageURL"];
            break;
          }
    }
    else
    {
      KODI_LOG(ADDON::LOG_ERROR, "Failed to parse SD Data");
      return false;
    }
  }
  else
  {
    KODI_LOG(ADDON::LOG_ERROR, "Failed to retrieve SD Data");
    return false;
  }
  return true;
}

void CEpg_Xml::DuplicateChannelCheck(HDHomeRunTuners::Tuner *pTuner)
{
  for (auto& jsonGuideItem : pTuner->Guide)
    if (jsonGuideItem["Guide"].size() == 0)
      for (const auto& jsonGuideItem2 : pTuner->Guide)
      {
        if (jsonGuideItem2["Guide"].size() == 0)
          continue;
        if (strcmp(jsonGuideItem["GuideName"].asString().c_str(),jsonGuideItem2["GuideName"].asString().c_str()) == 0)
        {
          jsonGuideItem["Guide"] = jsonGuideItem2["Guide"];
        }
      }
}

bool CEpg_Xml::XmlParseElement(HDHomeRunTuners::Tuner *pTuner, const rapidxml::xml_node<> *pRootNode, const char *strElement)
{
  Json::Value::ArrayIndex nCount;
  rapidxml::xml_node<> *pChannelNode;
  std::string strPreviousId;

  for(pChannelNode = pRootNode->first_node(strElement); pChannelNode; pChannelNode = pChannelNode->next_sibling(strElement))
  {
    std::string strId;
    if (strcmp(strElement, "channel") == 0)
    {
      if(!GetAttributeValue(pChannelNode, "id", strId))
        continue;
      std::string strName;
      GetNodeValue(pChannelNode, "display-name", strName);

      rapidxml::xml_node<> *pChannelLCN;
      std::vector<Json::Value*> vGuide;
      for(pChannelLCN = pChannelNode->first_node("LCN"); pChannelLCN; pChannelLCN = pChannelLCN->next_sibling("LCN"))
      {
        Json::Value& jsonChannel = CEpg_Xml::FindJsonValue(pTuner->Guide, "GuideNumber", pChannelLCN->value());
        if (jsonChannel.isNull())
          continue;
        Json::Value* jsonChannelPointer = &jsonChannel;
        vGuide.push_back(jsonChannelPointer);
      }
      channelMap[strId] = vGuide;
    }
    else if (strcmp(strElement, "programme") == 0)
    {
      if(!GetAttributeValue(pChannelNode, "channel", strId))
        continue;

      int iYear;
      Json::Value jsonFilter = Json::Value(Json::arrayValue);
      rapidxml::xml_node<> *pProgrammeCategory, *pProgrammeActors, *pProgrammeCredits, *pProgrammeDirectors;

      if (strPreviousId.empty())
      {
        strPreviousId = strId;
      }
      std::vector<Json::Value*> vGuide = channelMap[strId];

      std::string strEpData, strEpName, strCategory, strActors, strDirectors, strYear,
                  strEpNumSystem, strStartTime, strEndTime;
      GetAttributeValue(pChannelNode, "start", strStartTime);
      GetAttributeValue(pChannelNode, "stop", strEndTime);
      int iStartTime = CEpg_Xml::ParseDateTime(strStartTime);
      int iEndTime = CEpg_Xml::ParseDateTime(strEndTime);
      std::string strTitle, strSynopsis;
      GetNodeValue(pChannelNode, "title", strTitle);
      GetNodeValue(pChannelNode, "desc", strSynopsis);
      if(pChannelNode->first_node("episode-num"))
      {
        // only support xmltv_ns numbering scheme at this stage
        GetAttributeValue(pChannelNode->first_node("episode-num"), "system", strEpNumSystem);
        if (strcmp(strEpNumSystem.c_str(), "xmltv_ns") == 0)
          GetNodeValue(pChannelNode, "episode-num", strEpData);
      }
      if(pChannelNode->first_node("sub-title"))
        GetNodeValue(pChannelNode, "sub-title", strEpName);
      if(pChannelNode->first_node("date"))
      {
        GetNodeValue(pChannelNode, "date", strYear);
        iYear = std::stoi(strYear);
      }

      if(pChannelNode->first_node("category"))
        for(pProgrammeCategory = pChannelNode->first_node("category"); pProgrammeCategory; pProgrammeCategory = pProgrammeCategory->next_sibling("category"))
        {
          strCategory = pProgrammeCategory->value();
          jsonFilter.append(strCategory);
        }
      if(pChannelNode->first_node("credits"))
      {
        pProgrammeCredits = pChannelNode->first_node("credits");
        for(pProgrammeActors = pProgrammeCredits->first_node("actor"); pProgrammeActors; pProgrammeActors = pProgrammeActors->next_sibling("actor"))
        {
          if (strActors.empty())
            strActors = strActors + pProgrammeActors->value();
          else
            strActors = strActors + EPG_STRING_TOKEN_SEPARATOR + pProgrammeActors->value();
        }
        for(pProgrammeDirectors = pChannelNode->first_node("credits")->first_node("director"); pProgrammeDirectors; pProgrammeDirectors = pProgrammeDirectors->next_sibling("director"))
        {
          if (strDirectors.empty())
            strDirectors = strDirectors + pProgrammeDirectors->value();
          else
            strDirectors = strDirectors + EPG_STRING_TOKEN_SEPARATOR + pProgrammeDirectors->value();
        }
      }

      for (auto& it : vGuide)
      {
        Json::Value& jsonChannel = *it;
        // ToDo: imageurl
        //       SeriesID

        jsonChannel["Guide"][nCount]["StartTime"] = iStartTime;
        jsonChannel["Guide"][nCount]["EndTime"] = iEndTime;
        jsonChannel["Guide"][nCount]["Title"] = strTitle;
        if (!strEpName.empty())
          jsonChannel["Guide"][nCount]["EpisodeTitle"] = strEpName;
        if (!strEpData.empty())
          jsonChannel["Guide"][nCount]["EpisodeNumber"] = strEpData;
        jsonChannel["Guide"][nCount]["Synopsis"] = strSynopsis;
        jsonChannel["Guide"][nCount]["OriginalAirdate"] = 0; // not implemented
        jsonChannel["Guide"][nCount]["ImageURL"] = "";  // not implemented
        jsonChannel["Guide"][nCount]["SeriesID"] = tempSeriesId; // not implemented
        jsonChannel["Guide"][nCount]["Filter"] = jsonFilter;
        if (!(iYear == 0))
          jsonChannel["Guide"][nCount]["Year"] = iYear;
        if (!strActors.empty())
          jsonChannel["Guide"][nCount]["Cast"] = strActors;
        if (!strDirectors.empty())
          jsonChannel["Guide"][nCount]["Director"] = strDirectors;
        // Look at alternative for an actual series id
        // maybe other xml providers supply this as well
        tempSeriesId++;
      }
      if (strPreviousId ==  strId)
      {
        nCount++;
      }
      else
      {
        nCount = 0;
        strPreviousId = strId;
      }
    }
    else
    {
      KODI_LOG(ADDON::LOG_DEBUG, "element parse fail");
      return false;
    }
  }
  return true;
}

/*
 *  Create basic layout of Guide json for each channel listed by LineUp
 *
 */
void CEpg_Xml::PrepareGuide(HDHomeRunTuners::Tuner *pTuner)
{
  Json::Value::ArrayIndex nIndex;
  for (nIndex = 0; nIndex < pTuner->LineUp.size(); nIndex++)
  {
    pTuner->Guide[nIndex]["GuideNumber"] = pTuner->LineUp[nIndex]["GuideNumber"];
    pTuner->Guide[nIndex]["GuideName"] = pTuner->LineUp[nIndex]["GuideName"];
    pTuner->Guide[nIndex]["ImageURL"] = "";
    pTuner->Guide[nIndex]["Guide"] = Json::Value(Json::arrayValue);
  }
}

Json::Value& CEpg_Xml::FindJsonValue(Json::Value &Guide, const std::string& jsonElement, const std::string& searchData)
{
  Json::Value::ArrayIndex nIndex;
  for (auto& jsonGuide : Guide)
    if (strcmp(jsonGuide[jsonElement].asString().c_str(), searchData.c_str()) == 0)
      return jsonGuide;

  return const_cast<Json::Value&>(Json::Value::null);
}

/*
 * ParseDateTime pulled from iptvsimple and adapted slightly
 * Author: Anton Fedchin
 * Author: Pulse-Eight http://www.pulse-eight.com/
 * Original: http://github.com/afedchin/xbmc-addon-iptvsimple/
 */

int CEpg_Xml::ParseDateTime(const std::string& strDate)
{
  struct tm timeinfo;
  memset(&timeinfo, 0, sizeof(tm));
  char sign = '+';
  int hours = 0;
  int minutes = 0;

  sscanf(strDate.c_str(), "%04d%02d%02d%02d%02d%02d %c%02d%02d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec, &sign, &hours, &minutes);

  timeinfo.tm_mon  -= 1;
  timeinfo.tm_year -= 1900;
  timeinfo.tm_isdst = -1;

  time_t current_time;
  time(&current_time);
  long offset = 0;
#ifndef TARGET_WINDOWS
  offset = -localtime(&current_time)->tm_gmtoff;
#else
  _get_timezone(&offset);
#endif // TARGET_WINDOWS

  long offset_of_date = (hours * 60 * 60) + (minutes * 60);
  if (sign == '-')
  {
    offset_of_date = -offset_of_date;
  }

  return mktime(&timeinfo) - offset_of_date - offset;
}

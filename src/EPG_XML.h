#pragma once
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

#include <map>
#include <string>
#include <vector>

#include <json/json.h>
#include <rapidxml/rapidxml.hpp>

#include "HDHomeRunTuners.h"

class CEpg_Xml : public CEpgBase
{
public:
  CEpg_Xml() = default;
  virtual ~CEpg_Xml() = default;
  bool UpdateGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& xmltvlocation);

private:
  Json::Value& FindJsonValue(Json::Value &Guide, const std::string& jsonElement, const std::string& searchData);
  bool GzipInflate(const std::string &compressedBytes, std::string &uncompressedBytes);
  int ParseDateTime(const std::string& strDate);
  bool XmlParse(HDHomeRunTuners::Tuner *pTuner, char *xmlbuffer);
  void DuplicateChannelCheck(HDHomeRunTuners::Tuner *pTuner);
  void PrepareGuide(HDHomeRunTuners::Tuner *pTuner);
  bool UseSDIcons(HDHomeRunTuners::Tuner *pTuner);
  bool XmlParseElement(HDHomeRunTuners::Tuner *pTuner, const rapidxml::xml_node<> *pRootNode, const char *strElement);

  std::map<std::string, std::vector<Json::Value*>> channelMap;
  int tempSeriesId = 0;
};

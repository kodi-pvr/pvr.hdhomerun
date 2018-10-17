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

#include <string>

#include <json/json.h>

#include "HDHomeRunTuners.h"

class CEpg_SD : public CEpgBase
{
public:
  CEpg_SD() = default;
  virtual ~CEpg_SD() = default;
  bool UpdateGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& advancedguide);

private:
  unsigned long long GetEndTime(const Json::Value& jsonGuide);
  void InsertGuideData(Json::Value &Guide, const Json::Value& strInsertdata);
  void UpdateAdvancedGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& strUrl);
  bool UpdateBasicGuide(HDHomeRunTuners::Tuner *pTuner, const std::string& strUrl);
};

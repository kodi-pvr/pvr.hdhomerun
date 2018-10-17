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

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <json/json.h>

#include "HDHomeRunTuners.h"

class CEpgBase
{
public:
  virtual bool UpdateGuide(HDHomeRunTuners::Tuner*, const std::string&) = 0;
  static const std::string SD_ADVANCEDGUIDE;

protected:
  void AddGuideInfo(Json::Value&);
  static const std::string SD_GUIDEURL;
};

/*
 *  Author: John Cumming
 *  Created: 2016-02-02 Tue 21:21
 *  http://www.jsolutions.co.uk/C++/objectfactory.html
 */

// A helper class to register a factory function
class CRegistrar
{
public:
  CRegistrar(const std::string& className, std::function<CEpgBase*(void)> classFactoryFunction);
};

// A preprocessor define used by derived classes
#define REGISTER_CLASS(NAME, TYPE) static CRegistrar registrar(NAME, [](void) -> CEpgBase * { return new TYPE();});

// The factory - implements singleton pattern!
class CEpgFactory
{
public:
  // Get the single instance of the factory
  static CEpgFactory* Instance();
  // register a factory function to create an instance of className
  void RegisterFactoryFunction(const std::string& name, std::function<CEpgBase*(void)> classFactoryFunction);
  // create an instance of a registered class
  std::shared_ptr<CEpgBase> Create(const std::string& name);

private:
  CEpgFactory() {}
  // the registry of factory functions
  std::map<std::string, std::function<CEpgBase*(void)>> factoryFunctionRegistry;
};

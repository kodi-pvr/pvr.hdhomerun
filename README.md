[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](LICENSE.md)
[![Build and run tests](https://github.com/kodi-pvr/pvr.hdhomerun/actions/workflows/build.yml/badge.svg?branch=Piers)](https://github.com/kodi-pvr/pvr.hdhomerun/actions/workflows/build.yml)
[![Build Status](https://dev.azure.com/teamkodi/kodi-pvr/_apis/build/status/kodi-pvr.pvr.hdhomerun?branchName=Piers)](https://dev.azure.com/teamkodi/kodi-pvr/_build/latest?definitionId=61&branchName=Piers)
[![Build Status](https://jenkins.kodi.tv/view/Addons/job/kodi-pvr/job/pvr.hdhomerun/job/Piers/badge/icon)](https://jenkins.kodi.tv/blue/organizations/jenkins/kodi-pvr%2Fpvr.hdhomerun/branches/)

# HDHomeRun PVR

HDHomeRun PVR client addon for [Kodi](https://kodi.tv)

## Build instructions

### Linux

1. `git clone --branch master https://github.com/xbmc/xbmc.git`
2. `git clone --branch Nexus https://github.com/kodi-pvr/pvr.hdhomerun.git`
3. `cd pvr.hdhomerun && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.hdhomerun -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

### Windows

1. `set ROOT=%CD%`
2. `git clone --branch master https://github.com/xbmc/xbmc.git`
3. `git clone https://github.com/kodi-pvr/pvr.hdhomerun.git`
4. `cd pvr.hdhomerun`
5. `mkdir build`
6. `cd build`
7. `cmake -G "Visual Studio 14" -DADDONS_TO_BUILD=pvr.hdhomerun -DCMAKE_BUILD_TYPE=Debug -DADDON_SRC_PREFIX=%ROOT% -DCMAKE_INSTALL_PREFIX=%ROOT%\xbmc\addons -DCMAKE_USER_MAKE_RULES_OVERRIDE=%ROOT%\xbmc\cmake\scripts\windows\CFlagOverrides.cmake -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=%ROOT%\xbmc\cmake\scripts\windows\CXXFlagOverrides.cmake -DPACKAGE_ZIP=1 %ROOT%\xbmc\cmake\addons`
8. `cmake --build . --config Debug`

## Useful links

* [Kodi's PVR user support](https://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support](https://forum.kodi.tv/forumdisplay.php?fid=136)

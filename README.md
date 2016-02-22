# HDHomeRun PVR [![Build Status](https://travis-ci.org/kodi-pvr/pvr.hdhomerun.svg?branch=master)](https://travis-ci.org/kodi-pvr/pvr.hdhomerun) [![Build status](https://ci.appveyor.com/api/projects/status/dqyxemegfoku7hue?svg=true)](https://ci.appveyor.com/project/zcsizmadia/pvr-hdhomerun) [![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

HDHomeRun PVR client addon for [Kodi] (http://kodi.tv)

## Build instructions

### Linux

1. `git clone https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.hdhomerun.git`
3. `cd pvr.hdhomerun && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.hdhomerun -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/project/cmake/addons`
5. `make`

### Windows 

1. `set ROOT=%CD%`
2. `git clone https://github.com/xbmc/xbmc.git`
3. `git clone https://github.com/kodi-pvr/pvr.hdhomerun.git`
4. `cd pvr.hdhomerun`
5. `mkdir build`
6. `cd build`
7. `cmake -G "Visual Studio 14" -DADDONS_TO_BUILD=pvr.hdhomerun -DCMAKE_BUILD_TYPE=Debug -DADDON_SRC_PREFIX=%ROOT% -DCMAKE_INSTALL_PREFIX=%ROOT%\xbmc\addons -DCMAKE_USER_MAKE_RULES_OVERRIDE=%ROOT%\xbmc\project\cmake\scripts\windows\c-flag-overrides.cmake -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=%ROOT%\xbmc\project\cmake\scripts\windows\cxx-flag-overrides.cmake -DPACKAGE_ZIP=1 %ROOT%\xbmc\project\cmake\addons`
8. `cmake --build . --config Debug`

## Useful links

* [Kodi's PVR user support] (http://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support] (http://forum.kodi.tv/forumdisplay.php?fid=136)

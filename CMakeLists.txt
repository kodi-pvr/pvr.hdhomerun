cmake_minimum_required(VERSION 3.5)
project(pvr.hdhomerun)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(hdhomerun REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
                    ${JSONCPP_INCLUDE_DIRS}
                    ${HDHOMERUN_INCLUDE_DIRS})

set(DEPLIBS ${JSONCPP_LIBRARIES}
            ${HDHOMERUN_LIBRARIES})

set(PVRHDHOMERUN_SOURCES src/HDHomeRunTuners.cpp
                         src/Settings.cpp
                         src/Utils.cpp)

set(PVRHDHOMERUN_HEADERS src/HDHomeRunTuners.h
                         src/Settings.h
                         src/Utils.h)

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
  list(APPEND DEPLIBS iphlpapi)
  add_compile_options(/wd4996)
  if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    # if _WIN32_WINNT isn't defined before including hdhomerun.h then
    # it's defined as _WIN32_WINNT_VISTA which breaks build on UWP
    add_definitions(-D_WIN32_WINNT=0x0A00)
  endif()
endif()

build_addon(pvr.hdhomerun PVRHDHOMERUN DEPLIBS)

include(CPack)

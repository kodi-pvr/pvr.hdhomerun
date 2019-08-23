cmake_minimum_required(VERSION 3.5)
project(pvr.hdhomerun)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(hdhomerun REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${JSONCPP_INCLUDE_DIRS}
                    ${HDHOMERUN_INCLUDE_DIRS})

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${HDHOMERUN_LIBRARIES})

set(PVRHDHOMERUN_SOURCES src/client.cpp
                         src/HDHomeRunTuners.cpp
                         src/Utils.cpp)

set(PVRHDHOMERUN_HEADERS src/client.h
                         src/HDHomeRunTuners.h
                         src/Utils.h)

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
  list(APPEND DEPLIBS iphlpapi)
  if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    # if _WIN32_WINNT isn't defined before including hdhomerun.h then 
    # it's defined as _WIN32_WINNT_VISTA which breaks build on UWP
    add_definitions(-D_WIN32_WINNT=0x0A00)
  endif()
endif()

build_addon(pvr.hdhomerun PVRHDHOMERUN DEPLIBS)

include(CPack)

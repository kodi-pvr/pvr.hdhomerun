project(pvr.hdhomerun)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

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

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
  list(APPEND DEPLIBS iphlpapi)
endif()

build_addon(pvr.hdhomerun PVRHDHOMERUN DEPLIBS)

include(CPack)

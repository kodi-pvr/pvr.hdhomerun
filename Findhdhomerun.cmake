find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (HDHOMERUN hdhomerun)
endif()

if(NOT HDHOMERUN_FOUND)
  find_path(HDHOMERUN_INCLUDE_DIRS hdhomerun.h
            PATH_SUFFIXES hdhomerun libhdhomerun)
  find_library(HDHOMERUN_LIBRARIES hdhomerun)
endif()

  if (NOT HDHOMERUN_INCLUDE_DIRS AND WIN32)
    set(HDHOMERUN_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../libhdhomerun})
  endif()

  if (NOT HDHOMERUN_LIBRARIES AND WIN32)
    set(HDHOMERUN_LIBRARIES ${CMAKE_CURRENT_LIST_DIR}/../libhdhomerun/libhdhomerun.lib)
  endif()

  if (HDHOMERUN_INCLUDE_DIRS)
    MESSAGE(STATUS "Found HDHOMERUN_INCLUDE_DIRS at " ${HDHOMERUN_INCLUDE_DIRS})
  else()
    MESSAGE(FATAL_ERROR "Not found HDHOMERUN_INCLUDE_DIRS")
  endif()

  if (HDHOMERUN_LIBRARIES)
    MESSAGE(STATUS "Found HDHOMERUN_LIBRARIES    at " ${HDHOMERUN_LIBRARIES})
  else()
    MESSAGE(FATAL_ERROR "Not found HDHOMERUN_LIBRARIES")
  endif()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hdhomerun DEFAULT_MSG HDHOMERUN_LIBRARIES HDHOMERUN_INCLUDE_DIRS)

mark_as_advanced(HDHOMERUN_INCLUDE_DIRS HDHOMERUN_LIBRARIES)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (HDHOMERUN hdhomerun)
endif()

if(NOT HDHOMERUN_FOUND)
  find_path(HDHOMERUN_INCLUDE_DIRS hdhomerun.h
            PATH_SUFFIXES hdhomerun libhdhomerun)
  find_library(HDHOMERUN_LIBRARIES hdhomerun)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hdhomerun DEFAULT_MSG HDHOMERUN_LIBRARIES HDHOMERUN_INCLUDE_DIRS)

mark_as_advanced(HDHOMERUN_INCLUDE_DIRS HDHOMERUN_LIBRARIES)

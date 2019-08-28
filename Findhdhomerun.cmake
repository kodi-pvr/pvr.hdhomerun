find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_HDHOMERUN hdhomerun QUIET)
endif()

find_path(HDHOMERUN_INCLUDE_DIRS hdhomerun.h
                                 PATHS ${PC_HDHOMERUN_INCLUDEDIR}
                                 PATH_SUFFIXES hdhomerun libhdhomerun)
find_library(HDHOMERUN_LIBRARIES hdhomerun
                                 PATHS ${PC_HDHOMERUN_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hdhomerun REQUIRED_VARS HDHOMERUN_LIBRARIES HDHOMERUN_INCLUDE_DIRS)

mark_as_advanced(HDHOMERUN_INCLUDE_DIRS HDHOMERUN_LIBRARIES)

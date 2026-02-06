# FindLibobs.cmake
# Finds OBS Studio headers and library for plugin development
#
# Inputs:
#   OBS_SOURCE_DIR  - Path to obs-studio source checkout (for headers)
#   OBS_LIB_DIR     - Path to directory containing obs.lib
#
# Outputs:
#   OBS::libobs imported target

if(NOT OBS_SOURCE_DIR)
  message(FATAL_ERROR "OBS_SOURCE_DIR must be set to the obs-studio source directory")
endif()

if(NOT OBS_LIB_DIR)
  message(FATAL_ERROR "OBS_LIB_DIR must be set to the directory containing obs.lib")
endif()

# Verify paths exist
if(NOT EXISTS "${OBS_SOURCE_DIR}/libobs/obs-module.h")
  message(FATAL_ERROR "Cannot find obs-module.h in ${OBS_SOURCE_DIR}/libobs/")
endif()

if(NOT EXISTS "${OBS_LIB_DIR}/obs.lib")
  message(FATAL_ERROR "Cannot find obs.lib in ${OBS_LIB_DIR}/")
endif()

# Create imported target
if(NOT TARGET OBS::libobs)
  add_library(OBS::libobs SHARED IMPORTED)

  set_target_properties(OBS::libobs PROPERTIES
    IMPORTED_IMPLIB "${OBS_LIB_DIR}/obs.lib"
    IMPORTED_LOCATION "${OBS_LIB_DIR}/obs.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/libobs"
  )

  # OBS also needs the generated config headers — provide a config stub
  if(EXISTS "${OBS_SOURCE_DIR}/build/config")
    set_property(TARGET OBS::libobs APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/build/config"
    )
  endif()

  message(STATUS "Found libobs: ${OBS_LIB_DIR}/obs.lib")
  message(STATUS "  Headers: ${OBS_SOURCE_DIR}/libobs")
endif()

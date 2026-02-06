# OBS Plugin Bootstrap
# Based on the OBS Plugin Template bootstrap.cmake

# Set CMake policies
if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()

# --- Platform-specific defaults ---

if(WIN32)
  set(OS_WINDOWS TRUE)
elseif(APPLE)
  set(OS_MACOS TRUE)
else()
  set(OS_LINUX TRUE)
endif()

# --- Helper: compilerconfig ---

macro(compilerconfig)
endmacro()

# --- Helper: defaults ---

macro(defaults)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()
endmacro()

# --- Helper: helpers ---

macro(helpers)
endmacro()

# --- Find OBS ---

if(NOT DEFINED OBS_SOURCE_DIR AND NOT DEFINED CMAKE_PREFIX_PATH)
  message(
    WARNING
    "OBS_SOURCE_DIR or CMAKE_PREFIX_PATH not set. "
    "Set CMAKE_PREFIX_PATH to your OBS Studio install or build directory.")
endif()

find_package(libobs REQUIRED)

# --- set_target_properties_plugin ---

function(set_target_properties_plugin target)
  set_target_properties(${target} PROPERTIES PREFIX "")

  if(OS_WINDOWS)
    set_target_properties(${target} PROPERTIES SUFFIX ".dll")

    # Install to OBS plugin directory structure
    install(
      TARGETS ${target}
      LIBRARY DESTINATION obs-plugins/64bit
      RUNTIME DESTINATION obs-plugins/64bit)

    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
      DESTINATION data/obs-plugins/${target})
  endif()
endfunction()

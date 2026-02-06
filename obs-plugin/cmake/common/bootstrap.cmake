# OBS Plugin Bootstrap — simplified for CI and local builds

if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()

# Platform detection
if(WIN32)
  set(OS_WINDOWS TRUE)
elseif(APPLE)
  set(OS_MACOS TRUE)
else()
  set(OS_LINUX TRUE)
endif()

# Empty macros expected by CMakeLists.txt
macro(compilerconfig)
endmacro()

macro(defaults)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()
endmacro()

macro(helpers)
endmacro()

# Append our cmake/ directory to the module path so FindLibobs.cmake is found
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find OBS — try installed package first, then fall back to source-based find
find_package(libobs QUIET)

if(NOT libobs_FOUND AND NOT TARGET OBS::libobs)
  if(DEFINED OBS_SOURCE_DIR AND DEFINED OBS_LIB_DIR)
    find_package(Libobs REQUIRED)
  else()
    message(FATAL_ERROR
      "Could not find libobs. Either:\n"
      "  1) Set CMAKE_PREFIX_PATH to an OBS install/build directory, or\n"
      "  2) Set OBS_SOURCE_DIR and OBS_LIB_DIR for a source-based build.\n"
      "     OBS_SOURCE_DIR = path to obs-studio source (contains libobs/)\n"
      "     OBS_LIB_DIR    = path to directory with obs.lib")
  endif()
endif()

# Install helper
function(set_target_properties_plugin target)
  set_target_properties(${target} PROPERTIES PREFIX "")

  if(OS_WINDOWS)
    set_target_properties(${target} PROPERTIES SUFFIX ".dll")

    install(
      TARGETS ${target}
      LIBRARY DESTINATION obs-plugins/64bit
      RUNTIME DESTINATION obs-plugins/64bit)

    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
      DESTINATION data/obs-plugins/${target})
  endif()
endfunction()

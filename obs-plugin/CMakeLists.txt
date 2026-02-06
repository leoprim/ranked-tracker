cmake_minimum_required(VERSION 3.16...3.26)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(obs-sr-tracker VERSION 1.0.0)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

# --- Find dependencies ---

find_package(Tesseract REQUIRED)
find_package(CURL REQUIRED)

# --- Plugin sources ---

add_library(${CMAKE_PROJECT_NAME} MODULE)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.cpp
          src/sr-source.cpp
          src/ocr-engine.cpp
          src/api-client.cpp
          src/sr-source.h
          src/ocr-engine.h
          src/api-client.h
          src/plugin-support.h)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs Tesseract::libtesseract CURL::libcurl)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)

# --- Plugin install ---

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})

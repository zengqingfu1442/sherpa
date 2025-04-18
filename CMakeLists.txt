cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(sherpa)

# Remember to change
# scripts/conda-cpu/sherpa/meta.yaml
set(SHERPA_VERSION "1.3")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(NOT APPLE)
  set(SHERPA_RPATH_ORIGIN "$ORIGIN")
else()
  set(CMAKE_MACOSX_RPATH ON)
  set(SHERPA_RPATH_ORIGIN "@loader_path")
endif()

set(CMAKE_INSTALL_RPATH ${SHERPA_RPATH_ORIGIN})
set(CMAKE_BUILD_RPATH ${SHERPA_RPATH_ORIGIN})

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No CMAKE_BUILD_TYPE given, default to Release")
  set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ version to be used.")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

option(SHERPA_ENABLE_TESTS "Whether to build tests" OFF)
option(SHERPA_ENABLE_PORTAUDIO "Whether to build with portaudio" ON)
option(SHERPA_ENABLE_WEBSOCKET "Whether to build with websocket" ON)
option(SHERPA_ENABLE_GRPC "Whether to build with grpc" OFF)
option(BUILD_SHARED_LIBS "Whether to build shared libraries" ON)

message(STATUS "SHERPA_ENABLE_TESTS: ${SHERPA_ENABLE_TESTS}")
message(STATUS "SHERPA_ENABLE_PORTAUDIO: ${SHERPA_ENABLE_PORTAUDIO}")
message(STATUS "SHERPA_ENABLE_WEBSOCKET: ${SHERPA_ENABLE_WEBSOCKET}")
message(STATUS "SHERPA_ENABLE_GRPC: ${SHERPA_ENABLE_GRPC}")

if(BUILD_SHARED_LIBS AND MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

include(CheckIncludeFileCXX)
check_include_file_cxx(cxxabi.h SHERPA_HAVE_CXXABI_H)
check_include_file_cxx(execinfo.h SHERPA_HAVE_EXECINFO_H)

if(UNIX AND NOT APPLE)
  execute_process(COMMAND
    lsb_release -sd
    OUTPUT_VARIABLE SHERPA_OS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
elseif(APPLE)
  execute_process(COMMAND
    sw_vers -productName
    OUTPUT_VARIABLE _product_name
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(COMMAND
    sw_vers -productVersion
    OUTPUT_VARIABLE _product_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(COMMAND
    sw_vers -buildVersion
    OUTPUT_VARIABLE _build_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(SHERPA_OS "${_product_name} ${_product_version} ${_build_version}")
elseif(WIN32)
  execute_process(COMMAND
    wmic os get caption,version
    OUTPUT_VARIABLE SHERPA_OS_TWO_LINES
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  # Now SHERPA_OS_TWO_LINES contains something like
  #  Caption                          Version
  #  Microsoft Windows 10 Pro         10.0.18362
  string(REPLACE "\n" ";" SHERPA_OS_LIST ${SHERPA_OS_TWO_LINES})
  list(GET SHERPA_OS_LIST 1 SHERPA_OS)
else()
  set(SHERPA_OS "Unknown")
endif()

string(REGEX REPLACE "^\"+|\"+$" "" SHERPA_OS "${SHERPA_OS}")
message(STATUS "SHERPA_OS: ${SHERPA_OS}")
message(STATUS "CMake version: ${CMAKE_VERSION}")

if(WIN32)
  add_definitions(-DNOMINMAX) # Otherwise, std::max() and std::min() won't work
endif()

find_package(Git REQUIRED)
execute_process(COMMAND
  "${GIT_EXECUTABLE}" describe --always --abbrev=40
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE SHERPA_GIT_SHA1
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND
  "${GIT_EXECUTABLE}" log -1 --format=%ad --date=local
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE SHERPA_GIT_DATE
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(COMMAND
  "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE SHERPA_GIT_BRANCH
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "SHERPA_GIT_SHA1: ${SHERPA_GIT_SHA1}")
message(STATUS "SHERPA_GIT_DATE: ${SHERPA_GIT_DATE}")
message(STATUS "SHERPA_GIT_BRANCH: ${SHERPA_GIT_BRANCH}")


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wall ")
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -g ")
endif()

include(pybind11)
include(json)
# to prevent cmake from trying to link with system installed mkl since we not directly use it
# mkl libraries should be linked with pytorch already
# ref: https://github.com/pytorch/pytorch/blob/master/cmake/public/mkl.cmake
set(CMAKE_DISABLE_FIND_PACKAGE_MKL TRUE)
include(torch)
include(k2)
include(kaldifeat)
include(kaldi_native_io)
if(SHERPA_ENABLE_PORTAUDIO)
  include(portaudio)
endif()

if(SHERPA_ENABLE_WEBSOCKET OR SHERPA_ENABLE_GRPC)
  include(asio)
endif()

if(SHERPA_ENABLE_WEBSOCKET)
  include(websocketpp)
endif()

if(SHERPA_ENABLE_GRPC)
  include(grpc)
endif()

if(SHERPA_ENABLE_TESTS)
  enable_testing()
  include(googletest)
endif()

# TORCH_VERSION is defined in cmake/torch.cmake
configure_file(
  ${CMAKE_SOURCE_DIR}/sherpa/python/sherpa/torch_version.py.in
  ${CMAKE_SOURCE_DIR}/sherpa/python/sherpa/torch_version.py @ONLY
)

if(WIN32)
  # disable various warnings for MSVC
  # 4624: destructor was implicitly defined as deleted because a base class destructor is inaccessible or deleted
  set(disabled_warnings
      /wd4624
  )
  message(STATUS "Disabled warnings: ${disabled_warnings}")
  foreach(w IN LISTS disabled_warnings)
    string(APPEND CMAKE_CXX_FLAGS " ${w} ")
  endforeach()
endif()

include_directories(${CMAKE_SOURCE_DIR})

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

add_subdirectory(sherpa)

install(FILES
  ${PROJECT_SOURCE_DIR}/sherpa/python/sherpa/torch_version.py
  DESTINATION ./
)

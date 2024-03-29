cmake_minimum_required(VERSION 3.21)

# Respect CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

project(com.fredemmott.audiodevicelib VERSION 3.0.0 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL AND MSVC)
  # Better error messages
  add_compile_options(
    "/diagnostics:caret"
  )

  # Static CRT...
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

  # ... but dynamic UCRT
  add_link_options(
    "/DEFAULTLIB:ucrt$<$<CONFIG:Debug>:d>.lib" # include the dynamic UCRT
    "/NODEFAULTLIB:libucrt$<$<CONFIG:Debug>:d>.lib" # remove the static UCRT
  )
endif()

include(GNUInstallDirs)
add_subdirectory(src)
add_subdirectory(demos)

set(
  SOURCE_URL_TYPE
  GIT_REPOSITORY CACHE STRING "AudioDeviceLib.cmake: path of source repository"
)
set(
  SOURCE_URL
  https://github.com/fredemmott/AudioDeviceLib
  CACHE STRING "AudioDeviceLib.cmake: type of source repository"
)
set(
  SOURCE_REF_TYPE
  GIT_TAG
  CACHE STRING "AudioDeviceLib.cmake: type of source repository version id"
)
set(
  SOURCE_REF
  CACHE STRING "AudioDeviceLib.cmake: source repository version ID"
)
configure_file(
  AudioDeviceLib.cmake.in
  AudioDeviceLib.cmake
  @ONLY
  NEWLINE_STYLE UNIX
)
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/AudioDeviceLib.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
  DESTINATION
  ${CMAKE_INSTALL_DATADIR}
)

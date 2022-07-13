cmake_minimum_required(VERSION 3.21)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(com.fredemmott.audiodevicelib VERSION 0.1 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
  if(MSVC)
    add_compile_options(
      "/await"
      "/Zc:__cplusplus"
    )
    add_compile_definitions(
      "-DUNICODE=1"
      "-DWIN32_LEAN_AND_MEAN=1"
    )
  endif()
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
)
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/AudioDeviceLib.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
  DESTINATION
  ${CMAKE_INSTALL_DATADIR}
)

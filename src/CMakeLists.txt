if(WIN32)
  set(SOURCES AudioDevicesWindows.cpp)
endif()
if(APPLE)
  set(SOURCES AudioDevicesMacOS.cpp)
endif()

add_library(
  AudioDeviceLib
  STATIC
  ${SOURCES}
)
target_sources(
  AudioDeviceLib
  PUBLIC
  FILE_SET publicHeaders
  TYPE HEADERS
  BASE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/../include"
  FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/../include/AudioDevices/AudioDevices.h"
)

if(APPLE)
  find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
  find_library(COREAUDIO_FRAMEWORK CoreAudio)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
  target_link_libraries(
   AudioDeviceLib 
    ${AUDIOTOOLBOX_FRAMEWORK}
    ${COREAUDIO_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
  )
elseif(WIN32)
  target_link_libraries(AudioDeviceLib Winmm)
  target_compile_definitions(
    AudioDeviceLib
    PRIVATE
    "UNICODE=1"
    "WIN32_LEAN_AND_MEAN=1"
  )
  if(MSVC)
    target_compile_options(
      AudioDeviceLib
      PUBLIC
      "/await"
      "/Zc:__cplusplus"
    )
  endif()
endif()

set_target_properties(
  AudioDeviceLib
  PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
)

install(
  TARGETS
  AudioDeviceLib
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  COMPONENT AudioDeviceLib
)
install(
  TARGETS
  AudioDeviceLib
  FILE_SET publicHeaders
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT AudioDeviceLib
)

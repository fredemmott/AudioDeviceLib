if(WIN32)
  set(SOURCES AudioDevicesWindows.cpp StringEncoding.cpp)
endif()
if(APPLE)
  set(SOURCES AudioDevicesMacOS.cpp)
endif()

add_library(
  AudioDeviceLib
  STATIC
  ${SOURCES}
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
endif()

set_target_properties(
  AudioDeviceLib
  PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
)
file(GLOB HEADERS *.h)
set_target_properties(AudioDeviceLib PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
  TARGETS
  AudioDeviceLib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/AudioDevices"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

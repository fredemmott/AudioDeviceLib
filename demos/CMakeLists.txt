
add_executable(list-devices list-devices.cpp)
target_link_libraries(list-devices AudioDeviceLib)
set_target_properties(
  list-devices
  PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

install(TARGETS list-devices COMPONENT Demos)

if(WIN32)
  install(FILES "$<TARGET_PDB_FILE:list-devices>" TYPE BIN COMPONENT Demos)
endif()

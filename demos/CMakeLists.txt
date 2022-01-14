
add_executable(list-devices list-devices.cpp)
target_link_libraries(list-devices AudioDeviceLib)
set_target_properties(
  list-devices
  PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

install(TARGETS list-devices DESTINATION ${CMAKE_INSTALL_PREFIX}/demos)
if(WIN32)
  install(FILES "$<TARGET_PDB_FILE:list-devices>" DESTINATION ${CMAKE_INSTALL_PREFIX}/demos OPTIONAL)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(list-devices list-devices.cpp)
target_link_libraries(list-devices AudioDeviceLib)
install(TARGETS list-devices DESTINATION ${CMAKE_INSTALL_PREFIX}/demos)
if(WIN32)
  install(FILES "$<TARGET_PDB_FILE:list-devices>" DESTINATION ${CMAKE_INSTALL_PREFIX}/demos OPTIONAL)
endif()

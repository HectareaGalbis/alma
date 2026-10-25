
add_executable(alma2c main.cpp alma.cpp reader.cpp)
target_link_libraries(alma2c PRIVATE alma_gc alma_object alma_interpiler)

add_subdirectory(gc)
add_subdirectory(object)
add_subdirectory(interpiler)

include(GNUInstallDirs)
install(TARGETS alma2c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

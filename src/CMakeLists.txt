
add_executable(alma main.cpp alma.cpp reader.cpp)
target_link_libraries(alma PRIVATE alma_gc alma_object)

add_subdirectory(gc)
add_subdirectory(object)

# CMakeLists.txt para el directorio core
# Archivos compilados e instalados desde aquí

add_library(alma_gc STATIC gc.cpp)
target_include_directories(alma_gc PRIVATE ${CMAKE_PROJECT_SOURCE_DIR})

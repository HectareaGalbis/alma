
add_library(alma_object STATIC character.cpp character_macro.cpp cons.cpp environment.cpp function.cpp
  integer.cpp object.cpp package.cpp procedure.cpp special_operator.cpp string.cpp symbol.cpp
  type_system.cpp)
target_link_libraries(alma_object PUBLIC alma_interpiler)

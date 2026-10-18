add_library(alma_interpiler STATIC declaration.cpp emitter.cpp operators.cpp preprocessor.cpp record.cpp standard.cpp type.cpp)
target_link_libraries(alma_interpiler PUBLIC alma_object)

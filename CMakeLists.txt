

cmake_minimum_required(VERSION 3.10)
project(alma)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

function(colored_message color msg)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(White       "${Esc}[37m")
  
  if(color STREQUAL "red")
    set(ColorCode ${Red})
  elseif(color STREQUAL "green")
    set(ColorCode ${Green})
  elseif(color STREQUAL "blue")
    set(ColorCode ${Blue})
  elseif(color STREQUAL "yellow")
    set(ColorCode ${Yellow})
  else()
    set(ColorCode ${White})
  endif()

  message("${ColorCode}${msg}${ColourReset}")
endfunction()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  colored_message("blue" "-- Build type:  Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -g -rdynamic -Wall -Wextra -pedantic -Wshadow -fconcepts-diagnostics-depth=5")  
elseif(CMAKE_BUILD_TYPE STREQUAL "FastDebug")
  colored_message("blue" "-- Build type:  FastDebug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -O2 -rdynamic -Wall -Wextra -pedantic -Wshadow -fconcepts-diagnostics-depth=5 -fsanitize=address")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  colored_message("blue" "-- Build type:  Release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -O2 -Wall -Wextra -pedantic -Wshadow")
  add_compile_definitions(NDEBUG)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(src)

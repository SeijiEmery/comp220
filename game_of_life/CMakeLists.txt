cmake_minimum_required (VERSION 2.6)
project (assignment_11)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")

# Require c++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

include_directories(src ../assignment_10/src)    # HashTable.h

add_executable(game_of_life src/GameOfLife.cpp)
add_executable(game_of_life_map src/GameOfLifeMap.cpp)

add_custom_target(run
    COMMAND ./game_of_life
    DEPENDS game_of_life)

add_custom_target(run_map
    COMMAND ./game_of_life_map
    DEPENDS game_of_life_map)

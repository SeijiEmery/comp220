cmake_minimum_required (VERSION 2.6)
project (assignment_13)

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

add_executable(bfs   src/BFS.cpp)
add_executable(dfs   src/DFS.cpp)

add_custom_target(run_bfs
    COMMAND ./bfs
    DEPENDS bfs)

add_custom_target(run_dfs
    COMMAND cat test/input.txt | ./dfs > dfs_out.txt 
    DEPENDS dfs)

add_custom_target(test_bfs
    COMMAND cat test/input.txt | ./bfs > bfs_out.txt
    DEPENDS bfs)



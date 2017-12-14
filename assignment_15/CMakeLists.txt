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

add_executable(graph_shortest   src/GraphShortest.cpp)
add_executable(graph_cheapest   src/GraphCheapest.cpp)

add_custom_target(shortest
    COMMAND ./graph_shortest
    DEPENDS graph_shortest)

add_custom_target(cheapest
    COMMAND ./graph_cheapest
    DEPENDS graph_cheapest)

add_custom_target(run_test_shortest
    COMMAND cat ../test/input_shortest.txt | ./graph_shortest > output_shortest.txt
    DEPENDS graph_shortest)

add_custom_target(run_test_cheapest
    COMMAND cat ../test/input_cheapest.txt | ./graph_cheapest > output_cheapest.txt
    DEPENDS graph_cheapest)

add_custom_target(test_shortest
    COMMAND diff output_shortest.txt ../test/expected_shortest.txt
    DEPENDS run_test_shortest)

add_custom_target(test_cheapest
    COMMAND diff output_cheapest.txt ../test/expected_cheapest.txt 
    DEPENDS run_test_cheapest)

add_custom_target(test
    COMMAND echo "all tests passed!"
    DEPENDS test_shortest test_cheapest)

add_custom_target(run
    DEPENDS shortest cheapest)

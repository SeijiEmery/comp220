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


# executables: main program + testdriver
add_executable(simulation       src/BetterSimulation.cpp)
add_executable(itest            src/PriorityQueueTest.cpp)
add_executable(push_test        src/PriorityQueue.push.cpp)
add_executable(pop_test         src/PriorityQueue.pop.cpp)

add_custom_target(run
    COMMAND ./simulation
    DEPENDS simulation)

add_custom_target(setup
    COMMAND ./simulation setup
    DEPENDS simulation)

add_custom_target(push
    COMMAND ./push_test
    DEPENDS push_test)

add_custom_target(pop
    COMMAND ./pop_test
    DEPENDS pop_test)

add_custom_target(test
    COMMAND ./itest
    DEPENDS itest)

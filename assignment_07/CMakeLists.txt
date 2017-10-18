cmake_minimum_required (VERSION 2.6)
project (assignment_07)

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
add_executable(simulation   src/Simulation.cpp)
add_executable(testdriver   src/Queue.TestDriver.cpp)

add_custom_target(test
    COMMAND ./testdriver
    DEPENDS testdriver)

add_custom_target(run
    COMMAND ./simulation
    DEPENDS simulation)

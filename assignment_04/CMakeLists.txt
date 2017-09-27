cmake_minimum_required (VERSION 2.6)
project (assignment_03)
# enable_testing()

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

include_directories(src ../assignment_03/src)
add_executable(dvc4 
    src/DvcSchedule4.cpp)

add_executable(np
    src/NaiveParser.cpp)

# add_definitions(GROUP_SUBJECTS)
add_custom_target(rnp
    COMMAND ./np
    DEPENDS np)


add_custom_target(run
    COMMAND ./dvc4 data/dvc-schedule.txt
    DEPENDS dvc4)

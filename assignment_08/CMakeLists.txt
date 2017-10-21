cmake_minimum_required (VERSION 2.6)
project (assignment_08)

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

# Require c++17
# include(CheckCXXCompilerFlag)
# CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
# CHECK_CXX_COMPILER_FLAG("-std=c++1z" COMPILER_SUPPORTS_CXX1z)
# if(COMPILER_SUPPORTS_CXX17)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
# elseif(COMPILER_SUPPORTS_CXX1z)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
# else()
#         message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
# endif()

# import DynamicArray.h from assignment_03
include_directories(src ../assignment_03/src)

# executables: main program + testdriver
add_executable(dvc_test     src/dvc_version_3.cpp)

add_custom_target(run
    COMMAND ./dvc_test
    DEPENDS dvc_test)

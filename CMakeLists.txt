
cmake_minimum_required(VERSION 3.8...3.26)

project(
    TOOL
    VERSION 0.1.0
    DESCRIPTION "The Tired-Of-Objects Library!"
    LANGUAGES CXX)

if (WIN32)
    add_compile_definitions(TOOL_WINDOWS)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")

add_library(TOOL
    Source/Basics.cpp
    Source/Entry.cpp
    Source/Exception.cpp
    Source/Math.cpp
    Source/Memory.cpp
    Source/Quaternion.cpp
    Source/Random.cpp
    Source/Variadic.cpp    
)

target_include_directories(TOOL PUBLIC "Source")

target_compile_features(TOOL PUBLIC cxx_std_20)

set_target_properties(TOOL PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

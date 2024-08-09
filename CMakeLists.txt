
cmake_minimum_required(VERSION 3.8...3.26)

project(
    TOOL
    VERSION 0.1.0
    DESCRIPTION "The Tired-Of-Objects Library!"
    LANGUAGES CXX
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(TOOL_WINDOWS=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(TOOL_LINUX=1)
endif()

if (UNIX)
    add_compile_definitions(TOOL_UNIX=1)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces -march=native")
endif()

set(TOOL_SOURCE_DIR "source")
set(TOOL_INCLUDE_DIR "include")

# set(TOOL_OUT_DIR "${CMAKE_SOURCE_DIR}/out")
# set(TOOL_BIN_DIR "${TOOL_OUT_DIR}/lib")
# set(TOOL_TEST_DIR "${TOOL_OUT_DIR}/test")



# Main library

add_library(TOOL
    "${TOOL_SOURCE_DIR}/basics.cpp"
    "${TOOL_SOURCE_DIR}/entry.cpp"
    "${TOOL_SOURCE_DIR}/exception.cpp"
    "${TOOL_SOURCE_DIR}/mathematics.cpp"
    "${TOOL_SOURCE_DIR}/memory.cpp"
    "${TOOL_SOURCE_DIR}/quaternion.cpp"
    "${TOOL_SOURCE_DIR}/random.cpp"
    "${TOOL_SOURCE_DIR}/variadic.cpp"
    "${TOOL_SOURCE_DIR}/text.cpp"
    "${TOOL_SOURCE_DIR}/linking.cpp"
    "${TOOL_SOURCE_DIR}/threading.cpp"
    "${TOOL_SOURCE_DIR}/temporal.cpp"
)

target_include_directories(TOOL PUBLIC "${TOOL_INCLUDE_DIR}")
target_include_directories(TOOL PRIVATE "${TOOL_INCLUDE_DIR}/tool")

target_compile_features(TOOL PUBLIC 
    cxx_std_20
)

target_compile_definitions(TOOL PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
)

set_target_properties(TOOL PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)



# Test library

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    add_library(TOOLTestLib SHARED
    "${TOOL_SOURCE_DIR}/test/testlib.cpp"
    )

    target_link_libraries(TOOLTestLib TOOL)

    target_compile_features(TOOLTestLib PUBLIC cxx_std_20)

    set_target_properties(TOOLTestLib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    )



    # Test executable

    add_executable(TOOLTest
    "${TOOL_SOURCE_DIR}/test/test.cpp"
    )

    target_link_libraries(TOOLTest TOOL)
    # target_include_directories(TOOLTest ${TOOL_INCLUDE_DIRS})

    target_compile_features(TOOLTest PRIVATE cxx_std_20)

    set_target_properties(TOOLTest PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    )
    
endif()

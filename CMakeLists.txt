
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

# TOOL initializes nested aggregates without inner braces.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
endif()

set(TOOL_SOURCE_DIR "source")
set(TOOL_INCLUDE_DIR "include")

# set(TOOL_OUT_DIR "${CMAKE_SOURCE_DIR}/out")
# set(TOOL_BIN_DIR "${TOOL_OUT_DIR}/lib")
# set(TOOL_TEST_DIR "${TOOL_OUT_DIR}/test")



# Main library

add_library(TOOL
    "${TOOL_SOURCE_DIR}/basics.cpp"
    "${TOOL_SOURCE_DIR}/error.cpp"
    "${TOOL_SOURCE_DIR}/hashmap.cpp"
    "${TOOL_SOURCE_DIR}/intrinsics.cpp"
    "${TOOL_SOURCE_DIR}/mathematics.cpp"
    "${TOOL_SOURCE_DIR}/memory.cpp"
    "${TOOL_SOURCE_DIR}/quaternion.cpp"
    "${TOOL_SOURCE_DIR}/random.cpp"
    "${TOOL_SOURCE_DIR}/text.cpp"
    "${TOOL_SOURCE_DIR}/linking.cpp"
    "${TOOL_SOURCE_DIR}/threading.cpp"
    "${TOOL_SOURCE_DIR}/temporal.cpp"
    "${TOOL_SOURCE_DIR}/io.cpp"
)

target_include_directories(TOOL PUBLIC "${TOOL_INCLUDE_DIR}")

# Threads wait through WaitOnAddress on Windows and pthreads with futexes on Unix.
if (WIN32)
    target_link_libraries(TOOL PUBLIC Synchronization)
elseif (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(TOOL PUBLIC Threads::Threads)
endif()
target_include_directories(TOOL PRIVATE "${TOOL_INCLUDE_DIR}/tool")

target_compile_features(TOOL PUBLIC 
    cxx_std_20
)

# The instruction set is the consuming project's choice, made once for every file (see intrinsics.h). With it
# uniform, the GCC and Clang notes that 8-wide vectors pass differently with and without AVX have nothing to warn
# about.
target_compile_options(TOOL PUBLIC $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-psabi>)

target_compile_definitions(TOOL PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
)

target_compile_definitions(TOOL PUBLIC
    $<$<CONFIG:Debug>:TOOL_DEBUG_ASSERTS>
)

# Position-independent, since it links statically into shared libraries.
set_target_properties(TOOL PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)



# Test library

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

	# Core library
	
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



	# Module library
	
    add_library(TOOLTestModule SHARED
    "${TOOL_SOURCE_DIR}/test/testmodule.cpp"
    )

    target_link_libraries(TOOLTestModule TOOL TOOLTestLib)

    target_compile_features(TOOLTestModule PUBLIC cxx_std_20)

    set_target_properties(TOOLTestModule PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    )



    # Test executable

    add_executable(TOOLTest
    "${TOOL_SOURCE_DIR}/test/test.cpp"
    "${TOOL_SOURCE_DIR}/test/testhashmap.cpp"
    "${TOOL_SOURCE_DIR}/test/testintrinsics.cpp"
    "${TOOL_SOURCE_DIR}/test/testmathematics.cpp"
    "${TOOL_SOURCE_DIR}/test/testmatrix.cpp"
    "${TOOL_SOURCE_DIR}/test/testcolor.cpp"
    "${TOOL_SOURCE_DIR}/test/testrandom.cpp"
    "${TOOL_SOURCE_DIR}/test/testthreading.cpp"
    "${TOOL_SOURCE_DIR}/test/testio.cpp"
    "${TOOL_SOURCE_DIR}/test/testtext.cpp"
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

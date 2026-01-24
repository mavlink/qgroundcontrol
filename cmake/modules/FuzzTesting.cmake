# ============================================================================
# Fuzz Testing Configuration for QGroundControl
# ============================================================================
#
# Enables fuzz testing for file parsers and protocol handlers using libFuzzer
# (Clang) or AFL (GCC).
#
# Usage:
#   cmake -DQGC_ENABLE_FUZZING=ON -DCMAKE_CXX_COMPILER=clang++ ...
#   ninja fuzz-plan-parser
#   ninja fuzz-kml-parser
#
# The fuzzer will run until interrupted (Ctrl+C) or a crash is found.
# Crashes are saved to the corpus directory for reproduction.
#
# ============================================================================

option(QGC_ENABLE_FUZZING "Enable fuzz testing targets" OFF)

if(NOT QGC_ENABLE_FUZZING)
    return()
endif()

# Fuzzing requires Clang with libFuzzer
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "Fuzz testing requires Clang compiler. Fuzzing targets disabled.")
    return()
endif()

message(STATUS "Fuzz testing enabled")

# Create fuzz corpus directories
set(FUZZ_CORPUS_DIR "${CMAKE_BINARY_DIR}/fuzz-corpus")
set(FUZZ_CRASHES_DIR "${CMAKE_BINARY_DIR}/fuzz-crashes")
file(MAKE_DIRECTORY ${FUZZ_CORPUS_DIR})
file(MAKE_DIRECTORY ${FUZZ_CRASHES_DIR})

# Common fuzzer flags
set(FUZZER_FLAGS "-fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer")
set(FUZZER_LINK_FLAGS "-fsanitize=fuzzer,address,undefined")

# ============================================================================
# Fuzz Test Target Helper
# ============================================================================
# Creates a fuzz test target
#
# Usage:
#   add_fuzz_test(
#       NAME plan-parser
#       SOURCES fuzz_plan_parser.cc
#       SEED_CORPUS ${CMAKE_SOURCE_DIR}/test/MissionManager/*.plan
#   )

function(add_fuzz_test)
    cmake_parse_arguments(FUZZ "" "NAME" "SOURCES;SEED_CORPUS;LIBRARIES" ${ARGN})

    set(TARGET_NAME "fuzz-${FUZZ_NAME}")

    # Create the fuzz target executable
    add_executable(${TARGET_NAME} ${FUZZ_SOURCES})

    # Add fuzzer instrumentation
    target_compile_options(${TARGET_NAME} PRIVATE ${FUZZER_FLAGS})
    target_link_options(${TARGET_NAME} PRIVATE ${FUZZER_LINK_FLAGS})

    # Link against QGC libraries
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${CMAKE_PROJECT_NAME}
        ${FUZZ_LIBRARIES}
    )

    # Create corpus directory for this target
    set(TARGET_CORPUS_DIR "${FUZZ_CORPUS_DIR}/${FUZZ_NAME}")
    file(MAKE_DIRECTORY ${TARGET_CORPUS_DIR})

    # Copy seed corpus if provided
    if(FUZZ_SEED_CORPUS)
        file(GLOB SEED_FILES ${FUZZ_SEED_CORPUS})
        foreach(SEED_FILE ${SEED_FILES})
            get_filename_component(SEED_NAME ${SEED_FILE} NAME)
            configure_file(${SEED_FILE} ${TARGET_CORPUS_DIR}/${SEED_NAME} COPYONLY)
        endforeach()
    endif()

    # Create run target
    add_custom_target(run-${TARGET_NAME}
        COMMAND ${TARGET_NAME}
            ${TARGET_CORPUS_DIR}
            -artifact_prefix=${FUZZ_CRASHES_DIR}/${FUZZ_NAME}-
            -max_len=65536
            -timeout=10
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running fuzzer: ${TARGET_NAME}"
        VERBATIM
    )

    message(STATUS "  Fuzz target: ${TARGET_NAME}")
endfunction()

# ============================================================================
# Create Fuzz Test Directory
# ============================================================================
# The actual fuzz test harnesses are in test/Fuzz/

set(FUZZ_TEST_DIR "${CMAKE_SOURCE_DIR}/test/Fuzz")
if(EXISTS ${FUZZ_TEST_DIR})
    add_subdirectory(${FUZZ_TEST_DIR})
else()
    message(STATUS "  Fuzz test directory not found: ${FUZZ_TEST_DIR}")
    message(STATUS "  Create fuzz harnesses in test/Fuzz/ to enable fuzz targets")
endif()

# ============================================================================
# Documentation
# ============================================================================

message(STATUS "  Corpus directory: ${FUZZ_CORPUS_DIR}")
message(STATUS "  Crashes directory: ${FUZZ_CRASHES_DIR}")
message(STATUS "  Run a fuzzer with: ninja run-fuzz-<name>")

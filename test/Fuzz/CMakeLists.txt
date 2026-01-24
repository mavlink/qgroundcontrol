# ============================================================================
# QGroundControl Fuzz Tests
# ============================================================================
#
# Fuzz test harnesses for file parsers and protocol handlers.
#
# To run a fuzzer:
#   cmake -DQGC_ENABLE_FUZZING=ON -DCMAKE_CXX_COMPILER=clang++ -B build
#   cmake --build build --target fuzz-json-parser
#   ninja -C build run-fuzz-json-parser
#
# ============================================================================

# JSON Parser Fuzz Test
add_fuzz_test(
    NAME json-parser
    SOURCES FuzzJsonParser.cc
    SEED_CORPUS "${CMAKE_SOURCE_DIR}/test/MissionManager/*.json"
)

# Plan File Fuzz Test
add_fuzz_test(
    NAME plan-parser
    SOURCES FuzzPlanParser.cc
    SEED_CORPUS "${CMAKE_SOURCE_DIR}/test/MissionManager/*.plan"
)

# KML Parser Fuzz Test
add_fuzz_test(
    NAME kml-parser
    SOURCES FuzzKmlParser.cc
    SEED_CORPUS "${CMAKE_SOURCE_DIR}/test/MissionManager/*.kml"
)

# Waypoint Parser Fuzz Test
add_fuzz_test(
    NAME waypoint-parser
    SOURCES FuzzWaypointParser.cc
    SEED_CORPUS "${CMAKE_SOURCE_DIR}/test/MissionManager/*.waypoints"
)

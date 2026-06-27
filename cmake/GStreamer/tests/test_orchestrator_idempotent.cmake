cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Components)

set(GSTREAMER_APIS "stale-sentinel")
gstreamer_build_apis_and_deps(GSTREAMER_APIS _deps App)

qgc_test_assert_not_in_list("stale GSTREAMER_APIS overwritten" "stale-sentinel" GSTREAMER_APIS)
qgc_test_assert_in_list("recomputed GSTREAMER_APIS includes added component" "api_app" GSTREAMER_APIS)
qgc_test_pass("orchestrator recompute contract")

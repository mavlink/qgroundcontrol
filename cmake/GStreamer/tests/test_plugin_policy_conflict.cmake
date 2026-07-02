# Negative test (WILL_FAIL): a plugin in both the runtime-required and
# xcframework-skip lists must FATAL via _gstreamer_assert_policy_consistent.
cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include(Json)
include(PluginPolicy)
set(GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS foo)
set(GSTREAMER_RUNTIME_REQUIRED_PLUGINS foo)
_gstreamer_assert_policy_consistent()

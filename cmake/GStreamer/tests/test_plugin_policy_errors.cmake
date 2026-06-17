# Negative test (WILL_FAIL): gstreamer_plugins_for without QGC_BUILD_CONFIG_CONTENT must FATAL.
cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include(Json)
include(PluginPolicy)
gstreamer_plugins_for(PLATFORM "" OUT_VAR _x)

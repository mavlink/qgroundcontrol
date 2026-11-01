cmake_minimum_required(VERSION 3.25)
# qgc_config_read fails clearly when nothing loaded build-config.json; PluginPolicy.cmake loads it itself when
# included without the configure's copy.
include("${CMAKE_CURRENT_LIST_DIR}/../../modules/BuildConfigReader.cmake")
qgc_config_read(_x LIST gstreamer plugins common)

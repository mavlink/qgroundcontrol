# Negative test (WILL_FAIL): copying libs from a system prefix must FATAL.
cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include(Install)
gstreamer_install_libs(SOURCE_DIR "/usr/lib" DEST_DIR "x" EXTENSION "so")

cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}/../../modules")
include(Download)
gstreamer_get_package_url(windows_msvc_x64 1.27.0 _u)

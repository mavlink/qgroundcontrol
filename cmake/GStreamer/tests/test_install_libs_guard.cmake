cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include(Install)

if(WIN32)
    set(_system_lib_dir "C:/Windows/System32")
    set(_system_lib_ext "dll")
else()
    set(_system_lib_dir "/usr/lib")
    set(_system_lib_ext "so")
endif()

gstreamer_install_libs(SOURCE_DIR "${_system_lib_dir}" DEST_DIR "x" EXTENSION "${_system_lib_ext}")

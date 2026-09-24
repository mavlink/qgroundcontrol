cmake_minimum_required(VERSION 3.25)

project(QGCMoccacheLauncher LANGUAGES NONE)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()

include("${QGC_MODULE_DIR}/Helpers.cmake")

# qgc_config_moccache only needs an existing moc path; CMake stands in for it.
add_executable(Qt6::moc IMPORTED)
set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION "${CMAKE_COMMAND}")
qgc_config_moccache()

get_property(_launcher GLOBAL PROPERTY QGC_MOCCACHE_EXECUTABLE)
get_property(_stats GLOBAL PROPERTY QGC_MOCCACHE_STATS_EXECUTABLE)
file(WRITE "${CMAKE_BINARY_DIR}/moccache-scripts.txt" "${_launcher};${_stats}")

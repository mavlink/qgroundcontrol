if(NOT DEFINED QGC_COVERAGE_BUILD_DIR OR QGC_COVERAGE_BUILD_DIR STREQUAL "")
    message(FATAL_ERROR "QGC_COVERAGE_BUILD_DIR is required")
endif()

file(GLOB_RECURSE _qgc_coverage_data
    LIST_DIRECTORIES FALSE
    "${QGC_COVERAGE_BUILD_DIR}/*.gcda"
    "${QGC_COVERAGE_BUILD_DIR}/*.profraw"
    "${QGC_COVERAGE_BUILD_DIR}/*.profdata"
)
if(_qgc_coverage_data)
    file(REMOVE ${_qgc_coverage_data})
endif()

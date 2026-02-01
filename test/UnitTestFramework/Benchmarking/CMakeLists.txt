# ----------------------------------------------------------------------------
# nanobench (Microbenchmarking Framework)
# ----------------------------------------------------------------------------
CPMAddPackage(
    NAME nanobench
    GITHUB_REPOSITORY martinus/nanobench
    VERSION 4.3.11
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nanobench)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Benchmarking.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

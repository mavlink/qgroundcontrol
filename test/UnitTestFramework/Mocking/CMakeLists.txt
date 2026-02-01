# ----------------------------------------------------------------------------
# Trompeloeil (Mocking Framework)
# ----------------------------------------------------------------------------
CPMAddPackage(
    NAME trompeloeil
    GITHUB_REPOSITORY rollbear/trompeloeil
    VERSION 49
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE trompeloeil::trompeloeil)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Mocking.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

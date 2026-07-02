add_library(QGCConcurrency INTERFACE)

target_sources(QGCConcurrency
    INTERFACE
        AutoSuspendGuard.h
)

target_include_directories(QGCConcurrency INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCConcurrency)

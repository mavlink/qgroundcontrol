add_library(QGCDiagnostics INTERFACE)

target_sources(QGCDiagnostics
    INTERFACE
        EdgeTriggeredCounter.h
)

target_include_directories(QGCDiagnostics
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCDiagnostics)

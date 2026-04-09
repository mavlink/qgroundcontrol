add_library(QGCSecureMemory INTERFACE)

target_sources(QGCSecureMemory
    INTERFACE
        SecureMemory.h
)

target_include_directories(QGCSecureMemory
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCSecureMemory)

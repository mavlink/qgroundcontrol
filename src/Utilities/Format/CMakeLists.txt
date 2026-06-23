qt_add_library(QGCFormat STATIC
    QGCFormat.cc
    QGCFormat.h
)

target_link_libraries(QGCFormat
    PUBLIC
        Qt6::Core
)

target_include_directories(QGCFormat
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCFormat)

qt_add_library(QGCDatabase STATIC
    QGCSqlHelper.cc
    QGCSqlHelper.h
)

target_include_directories(QGCDatabase
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QGCDatabase
    PUBLIC
        Qt6::Core
        Qt6::Sql
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCDatabase)

if(NOT TARGET QGCWire)
    add_subdirectory(../Parsing/Wire "${CMAKE_CURRENT_BINARY_DIR}/Wire")
endif()

qt_add_library(QGCMath STATIC
    QGCMath.cc
    QGCMath.h
)

target_link_libraries(QGCMath
    PUBLIC
        Qt6::Core
    PRIVATE
        QGCWire
)

target_include_directories(QGCMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCMath)

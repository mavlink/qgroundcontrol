if(TARGET QGCLoggingCategory)
    return()
endif()

add_library(QGCLoggingCategory STATIC QGCLoggingCategory.cc QGCLoggingCategory.h)
add_library(QGC::LoggingCategory ALIAS QGCLoggingCategory)
target_include_directories(QGCLoggingCategory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QGCLoggingCategory PUBLIC Qt6::Core)

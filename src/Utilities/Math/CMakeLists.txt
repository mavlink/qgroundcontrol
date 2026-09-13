qt_add_library(QGCMath STATIC QGCMath.cc QGCMath.h CRC32.h)

target_link_libraries(QGCMath PUBLIC Qt6::Core)

target_compile_features(QGCMath PUBLIC cxx_std_20)

target_include_directories(QGCMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCMath)

# Header-only checksums, which the standalone GPS build uses without the application.
add_library(QGCChecksums INTERFACE)
add_library(QGC::Checksums ALIAS QGCChecksums)
target_compile_features(QGCChecksums INTERFACE cxx_std_20)
target_sources(QGCChecksums INTERFACE FILE_SET HEADERS FILES Checksums.h CRC32.h)

qt_add_library(QGCMath STATIC QGCMath.cc QGCMath.h)

target_link_libraries(QGCMath PUBLIC Qt6::Core QGCChecksums)

target_compile_features(QGCMath PUBLIC cxx_std_20)

target_include_directories(QGCMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET ${CMAKE_PROJECT_NAME})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCMath)
endif()

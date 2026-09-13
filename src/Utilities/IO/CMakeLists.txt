if(TARGET QGCIO)
    return()
endif()
if(NOT TARGET QGCClock)
    add_subdirectory(../Timing/Clock "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()

add_library(QGCIO STATIC TimestampedByteBuffer.cc DataRateTracker.cc)
add_library(QGC::IO ALIAS QGCIO)
target_link_libraries(QGCIO PUBLIC Qt6::Core QGCClock)

target_sources(QGCIO PUBLIC FILE_SET HEADERS FILES DataRateTracker.h ReadTimestamp.h TimestampedByteBuffer.h)

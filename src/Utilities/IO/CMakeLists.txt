if(TARGET QGCIO)
    return()
endif()

add_library(QGCIO STATIC TimestampedByteBuffer.cc TimestampedByteBuffer.h ReadTimestamp.h DataRateTracker.cc
                         DataRateTracker.h
)
add_library(QGC::IO ALIAS QGCIO)
target_include_directories(QGCIO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QGCIO PUBLIC Qt6::Core)

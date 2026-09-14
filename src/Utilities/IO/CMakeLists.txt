if(TARGET QGCIO)
    return()
endif()

add_library(QGCIO STATIC DataRateTracker.cc)
add_library(QGC::IO ALIAS QGCIO)
target_link_libraries(QGCIO PUBLIC Qt6::Core QGCClock)

target_sources(QGCIO PUBLIC FILE_SET HEADERS FILES DataRateTracker.h ReadTimestamp.h)

# ============================================================================
# GPS Core
# QGCGPSCore: the receiver protocols and transports, and the worker that runs them. It builds on Qt Core and Network
# without the application, Facts or QML, so the hardware runner and the protocol fuzzer build it standalone
# (test/GPS/Standalone). Each folder adds its sources and is an include root, public except Protocols/.
# ============================================================================

include("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Geo/GeographicLib.cmake")

qt_add_library(QGCGPSCore STATIC GPSInputMonitor.h GPSReceiverWorker.cc GPSReceiverWorker.h)
add_library(QGC::GPSCore ALIAS QGCGPSCore)
set_target_properties(QGCGPSCore PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSCore PUBLIC cxx_std_20)
target_include_directories(QGCGPSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSCore
    PUBLIC Qt6::Core Qt6::Network QGCChecksums QGCClock QGCTiming QGCWire
    PRIVATE GeographicLib::GeographicLib QGCLoggingCategory
)

foreach(directory Common NMEA RTCM Transport Protocols)
    add_subdirectory(${directory})
endforeach()

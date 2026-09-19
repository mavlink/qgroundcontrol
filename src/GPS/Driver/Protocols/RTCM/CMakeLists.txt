add_library(QGCGPSRTCMFramer INTERFACE)
add_library(QGC::GPSRTCMFramer ALIAS QGCGPSRTCMFramer)
target_compile_features(QGCGPSRTCMFramer INTERFACE cxx_std_20)
target_sources(QGCGPSRTCMFramer INTERFACE FILE_SET HEADERS FILES RTCMFramer.h)

add_library(QGCGPSNMEAUtils STATIC NMEAUtils.cc)
add_library(QGC::GPSNMEAUtils ALIAS QGCGPSNMEAUtils)
target_compile_features(QGCGPSNMEAUtils PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNMEAUtils
    PUBLIC Qt6::Core Qt6::Positioning
    PRIVATE QGCGPSNMEAProtocol
)
target_sources(QGCGPSNMEAUtils PUBLIC FILE_SET HEADERS FILES NMEAUtils.h)

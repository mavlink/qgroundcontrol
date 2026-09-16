add_library(QGCGPSNativeContracts INTERFACE)
add_library(QGC::GPSNativeContracts ALIAS QGCGPSNativeContracts)
target_compile_features(QGCGPSNativeContracts INTERFACE cxx_std_20)
target_sources(
    QGCGPSNativeContracts
    INTERFACE FILE_SET
              HEADERS
              FILES
              GPSAltitudeDatum.h
              GPSBaseStationConfig.h
              GPSConstellation.h
              GPSIOStatus.h
)

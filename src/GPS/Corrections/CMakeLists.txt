qt_add_library(
    QGCGPSCorrections
    STATIC
    GPSCorrectionDiagnostics.cc
    GPSCorrectionEventModel.cc
    GPSCorrectionLedger.cc
    GPSCorrectionRouter.cc
    GPSCorrectionSelector.cc
    GPSCorrectionSourceRegistration.cc
)
add_library(QGC::GPSCorrections ALIAS QGCGPSCorrections)
set_target_properties(QGCGPSCorrections PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
target_compile_features(QGCGPSCorrections PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSCorrections
    PUBLIC Qt6::Core
    PRIVATE QGCGPSRTCM QGCLoggingCategory
)
target_sources(
    QGCGPSCorrections
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSCorrectionDiagnostics.h
           GPSCorrectionEventModel.h
           GPSCorrectionFrame.h
           GPSCorrectionLedger.h
           GPSCorrectionRouter.h
           GPSCorrectionSelector.h
           GPSCorrectionSourceRegistration.h
)

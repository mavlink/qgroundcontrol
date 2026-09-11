add_library(
    QGCGPSNMEA STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEADecoderSession.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEADecoderSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEAPositionSource.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEAPositionSource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEASatelliteAdapter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEASatelliteAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEAStreamSplitter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEAStreamSplitter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEAUtils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEAUtils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NMEASentenceEnvelope.h
)
add_library(QGC::GPSNMEA ALIAS QGCGPSNMEA)
set_target_properties(QGCGPSNMEA PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSNMEA PUBLIC cxx_std_20)
target_include_directories(QGCGPSNMEA PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSNMEA
    PUBLIC QGCGPSCore QGCGPSNMEAProtocol
    PRIVATE QGCLoggingCategory
)

add_library(
    QGCGPSNTRIPSession STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPError.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPHttpDecoder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPHttpDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPSession.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPStream.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPStream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPTransportConfig.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPTransportConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPRequest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/NTRIPRequest.h
)
add_library(QGC::GPSNTRIPSession ALIAS QGCGPSNTRIPSession)
set_target_properties(QGCGPSNTRIPSession PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSNTRIPSession PUBLIC cxx_std_20)
target_include_directories(QGCGPSNTRIPSession PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSNTRIPSession
    PUBLIC QGCTiming Qt6::Core Qt6::Network
    PRIVATE QGCLoggingCategory
)

add_library(
    QGCGPSNTRIPNetwork STATIC
    NTRIPGgaProvider.cc
    NTRIPGgaProvider.h
    NTRIPTlsPolicy.h
    NTRIPConnectionStats.cc
    NTRIPConnectionStats.h
    NTRIPHttpResponse.cc
    NTRIPHttpResponse.h
    NTRIPHttpTransport.cc
    NTRIPHttpTransport.h
    NTRIPSourceTableController.cc
    NTRIPSourceTableController.h
    NTRIPSourceTable.cc
    NTRIPSourceTable.h
)
add_library(QGC::GPSNTRIPNetwork ALIAS QGCGPSNTRIPNetwork)
set_target_properties(QGCGPSNTRIPNetwork PROPERTIES AUTOMOC ON)
target_include_directories(QGCGPSNTRIPNetwork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSNTRIPNetwork
    PUBLIC QGCGPSNMEA QGCGPSNTRIPSession QGCGPSCorrections QGCIO Qt6::QmlIntegration
    PRIVATE QGCLoggingCategory
)

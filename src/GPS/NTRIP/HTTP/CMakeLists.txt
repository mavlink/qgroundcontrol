# ============================================================================
# NTRIP HTTP
# QGCNTRIPHttp: HTTP over TCP/TLS for NTRIP, the request/response codec and the socket session that carries one
# exchange. It builds on Qt Core and Network without the application, so the decoder fuzzer builds it standalone
# (test/GPS/Standalone).
# ============================================================================

qt_add_library(
    QGCNTRIPHttp
    STATIC
    NTRIPConfiguration.cc
    NTRIPConfiguration.h
    NTRIPError.h
    NTRIPHttpCodec.cc
    NTRIPHttpCodec.h
    NTRIPHttpSession.cc
    NTRIPHttpSession.h
    NTRIPTlsPolicy_p.h
)
add_library(QGC::NTRIPHttp ALIAS QGCNTRIPHttp)
set_target_properties(QGCNTRIPHttp PROPERTIES AUTOMOC ON)
target_compile_features(QGCNTRIPHttp PUBLIC cxx_std_20)
target_include_directories(QGCNTRIPHttp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCNTRIPHttp
    PUBLIC Qt6::Core Qt6::Network QGCTiming
    PRIVATE QGCLoggingCategory QGCNetworkClient
)

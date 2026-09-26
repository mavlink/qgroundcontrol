if(NOT TARGET QGCLoggingCategory)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Logging/Category"
                     "${CMAKE_CURRENT_BINARY_DIR}/LoggingCategory"
    )
endif()
if(NOT TARGET QGCClock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Timing/Clock" "${CMAKE_CURRENT_BINARY_DIR}/Clock")
endif()
if(NOT TARGET QGCNetworkClient)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Network/Client"
                     "${CMAKE_CURRENT_BINARY_DIR}/NetworkClient"
    )
endif()

# HTTP over TCP/TLS: the request/response codec and the socket session that carries one exchange.
qt_add_library(QGCGPSNTRIPHttp STATIC NTRIPConfiguration.cc NTRIPHttpCodec.cc NTRIPHttpSession.cc NTRIPTlsPolicy_p.h)
add_library(QGC::GPSNTRIPHttp ALIAS QGCGPSNTRIPHttp)
set_target_properties(QGCGPSNTRIPHttp PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSNTRIPHttp PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNTRIPHttp
    PUBLIC Qt6::Core Qt6::Network
    PRIVATE QGCClock QGCLoggingCategory QGCNetworkClient
)
target_sources(
    QGCGPSNTRIPHttp
    PUBLIC FILE_SET
           HEADERS
           FILES
           NTRIPConfiguration.h
           NTRIPError.h
           NTRIPHttpCodec.h
           NTRIPHttpSession.h
)

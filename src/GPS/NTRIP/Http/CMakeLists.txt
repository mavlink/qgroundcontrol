if(NOT TARGET QGCNetworkClient)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Utilities/Network/Client"
                     "${CMAKE_CURRENT_BINARY_DIR}/NetworkClient"
    )
endif()

qt_add_library(QGCGPSNTRIPHttp STATIC NTRIPConfiguration.cc NTRIPHttpCodec.cc)
add_library(QGC::GPSNTRIPHttp ALIAS QGCGPSNTRIPHttp)
set_target_properties(QGCGPSNTRIPHttp PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSNTRIPHttp PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNTRIPHttp
    PUBLIC Qt6::Core Qt6::Network
    PRIVATE QGCNetworkClient
)
target_sources(QGCGPSNTRIPHttp PUBLIC FILE_SET HEADERS FILES NTRIPConfiguration.h NTRIPError.h NTRIPHttpCodec.h)

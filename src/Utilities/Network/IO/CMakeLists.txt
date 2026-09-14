if(TARGET QGCNetworkIO)
    return()
endif()
add_library(QGCNetworkIO STATIC UdpIODevice.cc UdpForwarder.cc)
add_library(QGC::NetworkIO ALIAS QGCNetworkIO)
set_target_properties(QGCNetworkIO PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCNetworkIO
    PUBLIC Qt6::Core Qt6::Network QGCIO
    PRIVATE QGCLoggingCategory
)

target_sources(QGCNetworkIO PUBLIC FILE_SET HEADERS FILES UdpIODevice.h UdpForwarder.h)

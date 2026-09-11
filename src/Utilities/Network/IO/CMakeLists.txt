if(TARGET QGCNetworkIO)
    return()
endif()
add_library(QGCNetworkIO STATIC UdpIODevice.cc UdpIODevice.h UdpForwarder.cc UdpForwarder.h UdpPeer.h)
add_library(QGC::NetworkIO ALIAS QGCNetworkIO)
set_target_properties(QGCNetworkIO PROPERTIES AUTOMOC ON)
target_include_directories(QGCNetworkIO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCNetworkIO
    PUBLIC Qt6::Core Qt6::Network
    PRIVATE QGCLoggingCategory
)

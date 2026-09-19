if(TARGET QGCNetworkClient)
    return()
endif()

add_library(QGCNetworkClient STATIC QGCNetworkClient.cc)
add_library(QGC::NetworkClient ALIAS QGCNetworkClient)
target_link_libraries(QGCNetworkClient PUBLIC Qt6::Core Qt6::Network)
target_sources(QGCNetworkClient PUBLIC FILE_SET HEADERS FILES QGCNetworkClient.h)

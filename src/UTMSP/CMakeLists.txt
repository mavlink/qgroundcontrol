find_package(Threads REQUIRED)

add_library(UTMSP
        UTMSPLogger.h
        UTMSPRestInterface.h
        UTMSPRestInterface.cpp
        UTMSPBlenderRestInterface.h
        UTMSPBlenderRestInterface.cpp
        UTMSPAuthorization.h
        UTMSPAuthorization.cpp
        UTMSPNetworkRemoteIDManager.h
        UTMSPNetworkRemoteIDManager.cpp
        )

target_include_directories(UTMSP PUBLIC services )

target_link_libraries(UTMSP

PRIVATE
        libevents_parser
PUBLIC
        Qt5::Core
        Qt5::Location
        Qt5::Widgets
        Threads::Threads
        qgc
)

target_include_directories(USSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

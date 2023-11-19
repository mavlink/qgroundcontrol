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
        UTMSPAircraft.h
        UTMSPAircraft.cpp
        UTMSPFlightDetails.h
        UTMSPFlightDetails.cpp
        UTMSPOperator.h
        UTMSPOperator.cpp
        UTMSPFlightPlanManager.h
        UTMSPFlightPlanManager.cpp
        UTMSPServiceController.h
        UTMSPServiceController.cpp
        UTMSPVehicle.h
        UTMSPVehicle.cpp
        UTMSPManager.h
        UTMSPManager.cpp
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

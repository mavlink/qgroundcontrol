find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning)

qt_add_library(ADSB STATIC
    ADSBVehicle.cc
    ADSBVehicle.h
    ADSBTCPLink.cc
    ADSBTCPLink.h
    ADSBVehicleManager.cc
    ADSBVehicleManager.h
)

target_link_libraries(ADSB
    PRIVATE
        Qt6::Network
        Settings
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        QGC
        QmlControls
)

target_include_directories(ADSB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

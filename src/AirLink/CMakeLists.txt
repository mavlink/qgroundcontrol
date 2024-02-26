find_package(Qt6 COMPONENTS Core Network REQUIRED)

qt_add_library(AirLink STATIC
    AirlinkLink.cc
    AirlinkLink.h
    AirLinkManager.cc
    AirLinkManager.h
    AirLinkSettings.qml
)

target_link_libraries(AirLink
    PUBLIC
        Qt6::Core
        Qt6::Network
        FactSystem
        Settings
        qgc
)

target_include_directories(AirLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

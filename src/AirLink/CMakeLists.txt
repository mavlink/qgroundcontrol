set(SOURCES
    AirlinkLink.cc
    AirlinkLink.h
    AirLinkManager.cc
    AirLinkManager.h
    AirLinkSettings.qml
        )

find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Network REQUIRED)

add_library(AirLink ${SOURCES})
target_link_libraries(AirLink PUBLIC FactSystem)
target_link_libraries(AirLink PUBLIC Settings)
target_link_libraries(AirLink PUBLIC qgc)
target_link_libraries(AirLink PUBLIC Qt6::Core)
target_link_libraries(AirLink PUBLIC Qt6::Network)
target_include_directories(AirLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

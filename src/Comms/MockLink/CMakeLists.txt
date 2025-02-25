find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(MockLink STATIC)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(Qt6 REQUIRED COMPONENTS Positioning)

    target_sources(MockLink
        PRIVATE
            MockConfiguration.cc
            MockConfiguration.h
            MockLink.cc
            MockLink.h
            MockLinkFTP.cc
            MockLinkFTP.h
            MockLinkWorker.cc
            MockLinkWorker.h
            MockLinkMissionItemHandler.cc
            MockLinkMissionItemHandler.h
    )

    target_link_libraries(MockLink
        PRIVATE
            QGC
            Utilities
        PUBLIC
            Qt6::Core
            Qt6::Positioning
            Comms
            PX4FirmwarePlugin
            MAVLink
    )

    target_include_directories(MockLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

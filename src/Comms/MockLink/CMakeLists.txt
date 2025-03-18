find_package(Qt6 REQUIRED COMPONENTS Core)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(Qt6 REQUIRED COMPONENTS Positioning)

    target_sources(QGC
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

    target_link_libraries(QGC
        PUBLIC
            Qt6::Core
            Qt6::Positioning
    )

    target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

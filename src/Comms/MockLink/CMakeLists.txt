if(CMAKE_CONFIGURATION_TYPES)
    set(MOCKLINK_SOURCES
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockConfiguration.cc>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockConfiguration.h>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLink.cc>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLink.h>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLinkFTP.cc>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLinkFTP.h>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLinkWorker.cc>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLinkWorker.h>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLinkMissionItemHandler.cc>
        $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/MockLinkMissionItemHandler.h>
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MOCKLINK_SOURCES
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
else()
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${MOCKLINK_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

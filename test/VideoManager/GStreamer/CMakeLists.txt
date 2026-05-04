# TODO: QGC tests universally add to ${CMAKE_PROJECT_NAME} (test/Vehicle/, test/MissionManager/); a dedicated target would require reworking the whole test framework.

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GStreamerTest.cc
        GStreamerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

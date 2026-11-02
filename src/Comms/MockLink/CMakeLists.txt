if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    return()
endif()

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        MockConfiguration.cc
        MockConfiguration.h
        MockLink.cc
        MockLink.h
        MockLinkCamera.cc
        MockLinkCamera.h
        MockLinkFTP.cc
        MockLinkFTP.h
        MockLinkGimbal.cc
        MockLinkGimbal.h
        MockLinkWorker.cc
        MockLinkWorker.h
        MockLinkMissionItemHandler.cc
        MockLinkMissionItemHandler.h
        MockLinkPX4Calibration.cc
        MockLinkPX4Calibration.h
        MockVideoStreamServer.h
)

# The mock video stream is served by GStreamer; without it the null server serves none. QGCGStreamer is defined later
# (src/VideoManager), so the choice waits for generation.
set(_qgc_mock_video_server "${CMAKE_CURRENT_SOURCE_DIR}/MockVideoStreamServer.cc")
set(_qgc_mock_video_server_null "${CMAKE_CURRENT_SOURCE_DIR}/MockVideoStreamServerNull.cc")
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        $<IF:$<TARGET_EXISTS:QGCGStreamer>,${_qgc_mock_video_server},${_qgc_mock_video_server_null}>
)

set_source_files_properties(
    MockLink.General.MetaData.json
    PROPERTIES
        QT_RESOURCE_ALIAS General.MetaData.json
)
set_source_files_properties(
    MockLink.General.MetaData.json.xz
    PROPERTIES
        QT_RESOURCE_ALIAS General.MetaData.json.xz
)
set_source_files_properties(
    MockLink.Parameter.MetaData.json.xz
    PROPERTIES
        QT_RESOURCE_ALIAS Parameter.MetaData.json.xz
)
set_source_files_properties(
    MockLink.Parameter.MetaData.json
    PROPERTIES
        QT_RESOURCE_ALIAS Parameter.MetaData.json
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} mocklink_resources
    PREFIX "/MockLink"
    FILES
        GenericMockLink.params
        PX4MockLink.params
        MockLink.General.MetaData.json
        MockLink.General.MetaData.json.xz
        MockLink.Parameter.MetaData.json.xz
        MockLink.Parameter.MetaData.json
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

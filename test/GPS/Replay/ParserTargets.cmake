include_guard(GLOBAL)

if(NOT TARGET QGCGPSCore)
    find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning QmlIntegration Concurrent)
    set(QGC_NO_SERIAL_LINK ON)
    include("${CMAKE_CURRENT_LIST_DIR}/../../../src/GPS/Libraries.cmake")
endif()

# Compose parser dependencies for replay and fuzz consumers.
function(qgc_add_gps_parser_runtime target)
    add_library(${target} INTERFACE)
    target_link_libraries(${target} INTERFACE QGCLoggingCategory QGCGPSCore QGCGPSNMEA QGCGPSNTRIPSession
                                              QGCGPSCorrections
    )
    set_property(TARGET ${target} PROPERTY QGC_GPS_COMPONENT_TARGETS
                                           "QGCGPSCore;QGCGPSNMEA;QGCGPSNTRIPSession;QGCGPSCorrections;QGCTiming"
    )
endfunction()

# Add configured receiver and recording dependencies to a parser consumer.
function(qgc_add_gps_replay_driver target parser_target)
    add_library(${target} INTERFACE)
    target_link_libraries(${target} INTERFACE ${parser_target} QGCGPSDriver QGCGPSNative QGCGPSRecording QGCGPSReceiver
                                              QGCGPSRecordingController
    )
    target_include_directories(${target} INTERFACE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Driver")
    target_compile_definitions(${target} INTERFACE QGC_GPS_TEST_CLOCK)
endfunction()

# ============================================================================
# RTCM Module
# RTCM3 framing/parsing, UDP correction input, and MAVLink injection.
# UDP socket helpers live in Utilities/Network.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        RTCMMavlink.cc
        RTCMMavlink.h
        RTCMParser.cc
        RTCMParser.h
        RTCMUdpInput.cc
        RTCMUdpInput.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

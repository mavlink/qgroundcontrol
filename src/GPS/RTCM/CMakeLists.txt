# ============================================================================
# RTCM Module
# UDP correction input and MAVLink injection. RTCM3 framing/parsing lives in
# Utilities/RTCMParser; UDP socket helpers in Utilities/Network.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        RTCMMavlink.cc
        RTCMMavlink.h
        RTCMUdpInput.cc
        RTCMUdpInput.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

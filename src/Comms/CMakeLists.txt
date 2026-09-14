# ============================================================================
# Communications Module
# Handles all communication links (TCP, UDP, Serial, Bluetooth, etc.)
# ============================================================================

# ----------------------------------------------------------------------------
# Core Communication Sources
# ----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LinkConfiguration.cc
        LinkConfiguration.h
        LinkInterface.cc
        LinkInterface.h
        LinkManager.cc
        LinkManager.h
        LogReplayLink.cc
        LogReplayLink.h
        LogReplayLinkController.cc
        LogReplayLinkController.h
        MAVLinkProtocol.cc
        MAVLinkProtocol.h
        TCPLink.cc
        TCPLink.h
        UDPLink.cc
        UDPLink.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Uncomment to enable Qt socket debugging
# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QABSTRACTSOCKET_DEBUG)

# ============================================================================
# Optional Communication Features
# ============================================================================

# ----------------------------------------------------------------------------
# Serial Port Communication
# ----------------------------------------------------------------------------

add_subdirectory(Serial)

# ----------------------------------------------------------------------------
# Bluetooth Communication
# ----------------------------------------------------------------------------
add_subdirectory(Bluetooth)

# ============================================================================
# Communication Link Subdirectories
# ============================================================================

add_subdirectory(MockLink)

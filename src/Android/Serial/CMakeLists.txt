# ============================================================================
# Android Serial subsystem
# JNI-backed USB-host serial port plus its host-testable pure-logic pieces.
# ============================================================================

# The JNI-free trio below includes src/Comms/Serial headers; when serial is disabled (e.g. iOS) that dir
# returns early without publishing its include path, so skip these too.
if(QGC_NO_SERIAL_LINK)
    return()
endif()

# JNI-free pieces compiled on every platform so the host test suite (test/Comms/Serial) can exercise them:
#   * AndroidSerialPortRegistry — token->port map fencing JNI callbacks (UAF guard)
#   * AndroidSerialRxQueue       — RX flow-control accounting (backlog/epoch/warn latches)
#   * SerialPortInfoCodec        — packed USB-port enumeration decoder (wire layout shared with Java)
# The include dir is target-global, so the host tests and src/Comms/Serial reach these headers too.
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AndroidSerialPortRegistry.cc
        AndroidSerialPortRegistry.h
        AndroidSerialRxQueue.cc
        AndroidSerialRxQueue.h
        SerialPortInfoCodec.cc
        SerialPortInfoCodec.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ANDROID)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AndroidSerialPort.cc
        AndroidSerialPort.h
        AndroidSerialPortJni.cc
        AndroidSerialPortPrivate.h
        SerialWireConstants.h
        SerialWriteEngine.cc
        SerialWriteEngine.h
)

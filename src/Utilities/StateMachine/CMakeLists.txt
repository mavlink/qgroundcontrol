# ============================================================================
# State Machine Subsystem
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCFinalState.cc
        QGCFinalState.h
        QGCParallelState.cc
        QGCParallelState.h
        QGCState.cc
        QGCState.h
        QGCStateMachine.cc
        QGCStateMachine.h
        DelayState.cc
        DelayState.h
        FunctionState.cc
        FunctionState.h
        SendMavlinkCommandState.cc
        SendMavlinkCommandState.h
        SendMavlinkMessageState.cc
        SendMavlinkMessageState.h
        ShowAppMessageState.cc
        ShowAppMessageState.h
        WaitForMavlinkMessageState.cc
        WaitForMavlinkMessageState.h
        WatchForMavlinkMessageState.cc
        WatchForMavlinkMessageState.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

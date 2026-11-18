# ============================================================================
# State Machine Subsystem
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        # Core
        QGCStateMachine.cc
        QGCStateMachine.h
        QGCStateMachineEvent.cc
        QGCStateMachineEvent.h
        # States
        States/QGCAbstractState.cc
        States/QGCAbstractState.h
        States/QGCFinalState.cc
        States/QGCFinalState.h
        States/QGCState.cc
        States/QGCState.h
        States/AsyncFunctionState.cc
        States/AsyncFunctionState.h
        States/DelayState.cc
        States/DelayState.h
        States/FunctionState.cc
        States/FunctionState.h
        States/SendMavlinkCommandState.cc
        States/SendMavlinkCommandState.h
        States/SendMavlinkMessageState.cc
        States/SendMavlinkMessageState.h
        States/SkippableAsyncState.cc
        States/SkippableAsyncState.h
        States/WaitForMavlinkMessageState.cc
        States/WaitForMavlinkMessageState.h
        States/WaitForParamResponseState.cc
        States/WaitForParamResponseState.h
        States/WaitStateBase.cc
        States/WaitStateBase.h
        States/RetryableRequestMessageState.cc
        States/RetryableRequestMessageState.h
        # Transitions
        Transitions/QGCAbstractTransition.cc
        Transitions/QGCAbstractTransition.h
        Transitions/QGCSignalTransition.cc
        Transitions/QGCSignalTransition.h
        Transitions/GuardedTransition.cc
        Transitions/GuardedTransition.h
        Transitions/MachineEventTransition.cc
        Transitions/MachineEventTransition.h
        Transitions/RetryTransition.cc
        Transitions/RetryTransition.h
)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/States
        ${CMAKE_CURRENT_SOURCE_DIR}/Transitions
)

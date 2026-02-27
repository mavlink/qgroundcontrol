# ============================================================================
# State Machine Subsystem
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        # Core
        QGCStateMachine.cc
        QGCStateMachine.h
        QGCStateMachineEvent.cc
        QGCStateMachineEvent.h
        Helpers/StateContext.h
        Helpers/StateHistoryRecorder.cc
        Helpers/StateHistoryRecorder.h
        Helpers/StateMachineProfiler.cc
        Helpers/StateMachineProfiler.h
        Helpers/ErrorRecoveryBuilder.cc
        Helpers/ErrorRecoveryBuilder.h
        Helpers/ErrorHandlers.cc
        Helpers/ErrorHandlers.h
        Helpers/StateMachineLogger.cc
        Helpers/StateMachineLogger.h

        # States
        States/QGCAbstractState.cc
        States/QGCAbstractState.h
        States/QGCFinalState.cc
        States/QGCFinalState.h
        States/QGCHistoryState.cc
        States/QGCHistoryState.h
        States/ParallelState.cc
        States/ParallelState.h
        States/QGCState.cc
        States/QGCState.h
        States/AsyncFunctionState.cc
        States/AsyncFunctionState.h
        States/ConditionalState.cc
        States/ConditionalState.h
        States/DelayState.cc
        States/DelayState.h
        States/EventQueuedState.cc
        States/EventQueuedState.h
        States/FunctionState.cc
        States/FunctionState.h
        States/LoopState.h
        States/RetryState.cc
        States/RetryState.h
        States/FallbackChainState.cc
        States/FallbackChainState.h
        States/RollbackState.cc
        States/RollbackState.h
        States/CircuitBreakerState.cc
        States/CircuitBreakerState.h
        States/RequestMessageState.cc
        States/RequestMessageState.h
        States/SequenceState.cc
        States/SequenceState.h
        States/SendMavlinkCommandState.cc
        States/SendMavlinkCommandState.h
        States/SendMavlinkMessageState.cc
        States/SendMavlinkMessageState.h
        States/ShowAppMessageState.cc
        States/ShowAppMessageState.h
        States/SkippableAsyncState.cc
        States/SkippableAsyncState.h
        States/SubMachineState.cc
        States/SubMachineState.h
        States/WaitForMavlinkMessageState.cc
        States/WaitForMavlinkMessageState.h
        States/WaitForSignalState.cc
        States/WaitForSignalState.h
        States/WaitStateBase.cc
        States/WaitStateBase.h
        States/ProgressState.cc
        States/ProgressState.h
        States/RetryableRequestMessageState.cc
        States/RetryableRequestMessageState.h

        # Transitions
        Transitions/QGCAbstractTransition.cc
        Transitions/QGCAbstractTransition.h
        Transitions/QGCSignalTransition.cc
        Transitions/QGCSignalTransition.h
        Transitions/GuardedTransition.cc
        Transitions/GuardedTransition.h
        Transitions/InternalTransition.cc
        Transitions/InternalTransition.h
        Transitions/MachineEventTransition.cc
        Transitions/MachineEventTransition.h
        Transitions/NamedEventTransition.cc
        Transitions/NamedEventTransition.h
        Transitions/QGCEventTransition.cc
        Transitions/QGCEventTransition.h
        Transitions/RetryTransition.cc
        Transitions/RetryTransition.h
        Transitions/SignalDataTransition.cc
        Transitions/SignalDataTransition.h
        Transitions/TimeoutTransition.cc
        Transitions/TimeoutTransition.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Helpers
        ${CMAKE_CURRENT_SOURCE_DIR}/States
        ${CMAKE_CURRENT_SOURCE_DIR}/Transitions
)

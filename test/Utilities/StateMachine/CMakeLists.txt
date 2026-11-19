target_sources(
    QGCUnitTests
    PRIVATE
        states/StateTestCommon.h
)

add_qgc_tests(
    QGCStateMachineTest
    Helpers/ErrorRecoveryBuilderTest
    Helpers/StateContextTest
    Helpers/StateHistoryRecorderTest
    states/FunctionStateTest
    states/DelayStateTest
    states/AsyncFunctionStateTest
    states/WaitForSignalStateTest
    states/ConditionalStateTest
    states/SkippableAsyncStateTest
    states/ParallelStateTest
    states/SubMachineStateTest
    states/QGCStateTest
    states/QGCFinalStateTest
    states/QGCHistoryStateTest
    states/ProgressStateTest
    LABELS Unit Utilities
)
add_qgc_tests(states/RetryableRequestMessageStateTest LABELS Integration Vehicle Utilities)
add_qgc_tests(
    states/WaitStateBaseTest
    states/QGCAbstractStateTest
    states/ShowAppMessageStateTest
    states/RequestMessageStateTest
    states/SendMavlinkCommandStateTest
    states/SendMavlinkMessageStateTest
    states/WaitForMavlinkMessageStateTest
    states/AdditionalStatesCoverageTest
    LABELS Unit Utilities
)

target_sources(
    QGCUnitTests
    PRIVATE
        transitions/TransitionTestCommon.cc
        transitions/TransitionTestCommon.h
)

add_qgc_tests(
    transitions/GuardedTransitionTest
    transitions/InternalTransitionTest
    transitions/MachineEventTransitionTest
    transitions/NamedEventTransitionTest
    transitions/SignalDataTransitionTest
    transitions/QGCSignalTransitionTest
    transitions/RetryTransitionTest
    transitions/TimeoutTransitionTest
    transitions/AdditionalTransitionsCoverageTest
    LABELS Unit Utilities
)

# The tests in transitions/ include StateTestCommon.h too.
target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/states)

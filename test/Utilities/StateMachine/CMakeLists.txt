# ============================================================================
# State Machine Unit Tests
# Tests for state machine states and transitions
# ============================================================================

# ----------------------------------------------------------------------------
# Root-level tests
# ----------------------------------------------------------------------------
target_sources(
    QGCUnitTests
    PRIVATE
        QGCStateMachineTest.cc
        QGCStateMachineTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Helper Tests
# ----------------------------------------------------------------------------
target_sources(
    QGCUnitTests
    PRIVATE
        Helpers/SignalSenderShapes.cc
)

# ----------------------------------------------------------------------------
# State Tests
# ----------------------------------------------------------------------------
target_sources(
    QGCUnitTests
    PRIVATE
        states/StateTestCommon.h
        states/AsyncFunctionStateTest.cc
        states/AsyncFunctionStateTest.h
        states/RetryableRequestMessageStateTest.cc
        states/RetryableRequestMessageStateTest.h
        states/WaitStateBaseTest.cc
        states/WaitStateBaseTest.h
        states/QGCStateTest.cc
        states/QGCStateTest.h
        states/SendMavlinkCommandStateTest.cc
        states/SendMavlinkCommandStateTest.h
        states/SendMavlinkMessageStateTest.cc
        states/SendMavlinkMessageStateTest.h
        states/AdditionalStatesCoverageTest.cc
        states/AdditionalStatesCoverageTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/states)

# ----------------------------------------------------------------------------
# Transition Tests
# ----------------------------------------------------------------------------
target_sources(
    QGCUnitTests
    PRIVATE
        transitions/MachineEventTransitionTest.cc
        transitions/MachineEventTransitionTest.h
        transitions/AdditionalTransitionsCoverageTest.cc
        transitions/AdditionalTransitionsCoverageTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/transitions)

add_qgc_test(QGCStateMachineTest LABELS Unit Utilities)
add_qgc_test(AsyncFunctionStateTest LABELS Unit Utilities)
add_qgc_test(QGCStateTest LABELS Unit Utilities)
add_qgc_test(RetryableRequestMessageStateTest LABELS Integration Vehicle Utilities)
add_qgc_test(SendMavlinkCommandStateTest LABELS Integration Vehicle Utilities)
add_qgc_test(SendMavlinkMessageStateTest LABELS Integration Vehicle Utilities)
add_qgc_test(WaitStateBaseTest LABELS Unit Utilities)
add_qgc_test(MachineEventTransitionTest LABELS Unit Utilities)
add_qgc_test(AdditionalStatesCoverageTest LABELS Unit Utilities)
add_qgc_test(AdditionalTransitionsCoverageTest LABELS Unit Utilities)

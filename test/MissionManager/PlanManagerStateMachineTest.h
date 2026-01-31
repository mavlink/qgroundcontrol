#pragma once

#include "UnitTest.h"
#include "PlanManagerStateMachine.h"
#include "PlanManager.h"

class MultiSignalSpy;

class PlanManagerStateMachineTest : public UnitTest
{
    Q_OBJECT

public:
    PlanManagerStateMachineTest(void);

private slots:
    void cleanup(void);

    void _testInitialState();
    void _testStartRead();
    void _testStartWrite();
    void _testStartRemoveAll();
    void _testCancelTransaction();
    void _testInProgressTracking();
    void _testConcurrentOperationsPrevented();
    void _testReadStateTransitions();
    void _testWriteStateTransitions();

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _setupSignalSpy();

    PlanManager* _planManager = nullptr;
    PlanManagerStateMachine* _stateMachine = nullptr;

    typedef enum {
        transactionCompleteSignalIndex = 0,
        readCompleteSignalIndex,
        writeCompleteSignalIndex,
        removeAllCompleteSignalIndex,
        progressChangedSignalIndex,
        errorOccurredSignalIndex,
        maxSignalIndex
    } StateMachineSignalIndex_t;

    typedef enum {
        transactionCompleteSignalMask = 1 << transactionCompleteSignalIndex,
        readCompleteSignalMask = 1 << readCompleteSignalIndex,
        writeCompleteSignalMask = 1 << writeCompleteSignalIndex,
        removeAllCompleteSignalMask = 1 << removeAllCompleteSignalIndex,
        progressChangedSignalMask = 1 << progressChangedSignalIndex,
        errorOccurredSignalMask = 1 << errorOccurredSignalIndex,
    } StateMachineSignalMask_t;

    MultiSignalSpy* _multiSpy = nullptr;
    static const size_t _cSignals = maxSignalIndex;
    const char* _rgSignals[_cSignals];

    static const int _signalWaitTime = PlanManager::_ackTimeoutMilliseconds * PlanManager::_maxRetryCount * 2;
};

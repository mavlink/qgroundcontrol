#pragma once

#include "PortableTest.h"

/// Tests for QGC::runOnWorker(): a function on a worker thread whose result is handed to the thread of an object
class WorkerJobTest : public PortableTest
{
    Q_OBJECT

private slots:
    void _testDoneGetsTheOnlyResultOnContextThread();
    void _testDoneWaitsForTheEventLoop();
    UT_PARAMETERIZED_TEST(_testDeletingJobOrContextDropsResult);
    void _testDoneDeletesContext();
    void _testDoneStartsNextJob();
};

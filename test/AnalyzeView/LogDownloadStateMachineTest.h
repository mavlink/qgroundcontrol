#pragma once

#include "UnitTest.h"

class LogDownloadStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testStartListRequest();
    void _testHandleLogEntry();
    void _testStartDownload();
    void _testCancel();
    void _testListRequestWorkflow();
};

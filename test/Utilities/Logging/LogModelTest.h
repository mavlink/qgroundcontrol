#pragma once

#include "UnitTest.h"

class LogModel;

class LogModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testInitialState();
    void _testAppendEntry();
    void _testAppendMultiple();
    void _testClear();
    void _testRoleNames();
    void _testDataAccess();
    void _testMaxEntries();
    void _testAllFormatted();
    void _testCountChanged();

private:
    LogModel* _model = nullptr;
};

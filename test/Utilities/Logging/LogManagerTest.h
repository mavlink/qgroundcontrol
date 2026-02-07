#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

class LogManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testSingleton();
    void _testModel();
    void _testDiskLogging();
    void _testRemoteLogging();
    void _testSetLogDirectory();
    void _testClear();
    void _testFlush();
    void _testExportToFile();
    void _testErrorState();

private:
    QTemporaryDir* _tempDir = nullptr;
};

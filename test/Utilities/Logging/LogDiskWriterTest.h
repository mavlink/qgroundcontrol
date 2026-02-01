#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

class LogDiskWriter;

class LogDiskWriterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testInitialState();
    void _testSetFilePath();
    void _testEnableDisable();
    void _testWriteEntry();
    void _testWriteMultiple();
    void _testFlush();
    void _testMaxPendingEntries();
    void _testFileRotation();
    void _testErrorHandling();

private:
    LogDiskWriter* _writer = nullptr;
    QTemporaryDir* _tempDir = nullptr;
};

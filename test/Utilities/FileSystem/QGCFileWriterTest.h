#pragma once

#include "UnitTest.h"

class QGCFileWriter;

class QGCFileWriterTest : public UnitTest
{
    Q_OBJECT

public:
    QGCFileWriterTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    void _testInitialState();
    void _testSetFilePath();
    void _testStartStop();
    void _testWriteString();
    void _testWriteByteArray();
    void _testFlush();
    void _testFileSizeCallback();
    void _testPreOpenCallback();
    void _testErrorHandling();
    void _testMaxPendingBytes();
    void _testReopenOnPathChange();

private:
    QGCFileWriter* _writer = nullptr;
    QString _testFilePath;
};

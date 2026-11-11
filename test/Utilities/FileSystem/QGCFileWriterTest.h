#pragma once

#include "UnitTest.h"

/// Tests for QGCFileWriter (background file appender)
class QGCFileWriterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFlushWritesQueuedData();
    void _testFlushAfterManyWrites();
    void _testConsecutiveFlushes();
    void _testRecoversAfterError_data();
    void _testRecoversAfterError();
    void _testDestructorWritesQueuedData();
    void _testCloseWritesQueuedDataAndNextWriteReopens();
    void _testChangingPathSwitchesFile();
    void _testAppendsAndReportsFileSize();
    void _testWritesKeepQueueOrder();
    void _testWritesOverPendingLimitAreDropped();
    void _testWriteWithoutPathIsDropped_data();
    void _testWriteWithoutPathIsDropped();

private:
    static QByteArray _fileContents(const QString& path);
};

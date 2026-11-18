#pragma once

#include <optional>

#include <QtCore/QTemporaryDir>

#include "UnitTest.h"

/// Tests for QGCFileWriter (background file appender)
class QGCFileWriterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testFlushWritesQueuedData();
    void _testFlushAfterManyWrites();
    void _testConsecutiveFlushes();
    void _testRecoversAfterError_data();
    void _testRecoversAfterError();
    void _testCloseReportsWriteError_data();
    void _testCloseReportsWriteError();
    void _testWriteWhileClosingAfterError();
    void _testDestructorWritesQueuedData();
    void _testCloseWritesQueuedDataAndNextWriteReopens();
    void _testChangingPathSwitchesFile();
    void _testAppendsAndReportsFileSize();
    void _testWritesKeepQueueOrder();
    void _testWritesOverPendingLimitAreDropped();
    void _testWriteWithoutPathIsDropped_data();
    void _testWriteWithoutPathIsDropped();

private:
    std::optional<QTemporaryDir> _tempDir;  ///< Exists from init() to cleanup()
    QString _path;                          ///< A file in _tempDir that does not exist when a test starts
};

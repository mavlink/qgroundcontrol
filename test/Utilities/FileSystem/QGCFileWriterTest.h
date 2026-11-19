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
    UT_PARAMETERIZED_TEST(_testRecoversAfterError);
    UT_PARAMETERIZED_TEST(_testCloseReportsWriteError);
    void _testWriteWhileClosingAfterError();
    void _testDestructorWritesQueuedData();
    void _testCloseWritesQueuedDataAndNextWriteReopens();
    void _testChangingPathSwitchesFile();
    void _testAppendsAndReportsFileSize();
    void _testWritesKeepQueueOrder();
    void _testWritesOverPendingLimitAreDropped();
    UT_PARAMETERIZED_TEST(_testWriteWithoutPathIsDropped);

private:
    std::optional<QTemporaryDir> _tempDir;  ///< Exists from init() to cleanup()
    QString _path;                          ///< A file in _tempDir that does not exist when a test starts
};

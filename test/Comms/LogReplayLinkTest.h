#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "UnitTest.h"

/// Tests for LogReplayWorker tlog file loading. Logs with uninterpretable trailing
/// bytes (e.g. not closed cleanly due to crash/power loss) must still replay (issue #14210).
class LogReplayLinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCleanLogLoads();
    void _testTrailingBytesIgnored_data();
    void _testTrailingBytesIgnored();
    void _testGarbageOnlyLogFails();

private:
    QString _writeLogFile(const QByteArray& contents);

    QTemporaryDir _tempDir;
};

#include "BluetoothEmulatedAdapterTest.h"

#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtTest/QTest>

void BluetoothEmulatedAdapterTest::_testOptionalEmulatorHarness()
{
    const QString enable = qEnvironmentVariable("QGC_TEST_BLUETOOTH_EMULATION");
    if (enable != QStringLiteral("1")) {
        QSKIP("Set QGC_TEST_BLUETOOTH_EMULATION=1 to run emulator harness checks");
    }

    // Default to BlueZ virtual HCI emulator. Override with:
    // QGC_TEST_BLUETOOTH_EMULATOR_CMD='your command'
    const QString command = qEnvironmentVariable("QGC_TEST_BLUETOOTH_EMULATOR_CMD", QStringLiteral("btvirt -l2"));

    QProcess process;
    process.setProgram(QStringLiteral("/bin/sh"));
    process.setArguments({QStringLiteral("-lc"), command});
    process.start();

    if (!process.waitForStarted(5000)) {
        const QByteArray skipMsg = QStringLiteral("Failed to start emulator command: %1").arg(command).toUtf8();
        QSKIP(skipMsg.constData());
    }

    QTest::qWait(1000);

    if (process.state() != QProcess::Running) {
        (void) process.waitForFinished(2000);
        const QString stderrText = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        const QString stdoutText = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        const QString reason = !stderrText.isEmpty() ? stderrText : stdoutText;

        const QByteArray skipMsg = QStringLiteral("Emulator exited early; likely missing caps/tools. cmd='%1' reason='%2'")
                                       .arg(command, reason)
                                       .toUtf8();
        QSKIP(skipMsg.constData());
    }

    process.terminate();
    if (!process.waitForFinished(3000)) {
        process.kill();
        (void) process.waitForFinished(1000);
    }

    QCOMPARE(process.state(), QProcess::NotRunning);
}

UT_REGISTER_TEST(BluetoothEmulatedAdapterTest, TestLabel::Integration, TestLabel::Comms)

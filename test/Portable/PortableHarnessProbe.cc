#include "PortableHarnessProbe.h"

#include <cstdio>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>

// A category as Qt makes them: its debug and info messages are on unless the harness turns them off
Q_LOGGING_CATEGORY(PortableHarnessProbeLog, "Test.PortableHarnessProbe")

// Fails: cleanup() finds a message that was neither expected nor ignored, and dumps the context of that failure
void PortableHarnessProbe::_unexpectedWarning()
{
    qCWarning(PortableHarnessProbeLog) << "a warning that the test did not expect";
}

// Fails: cleanup() finds the expectation that was not verified
void PortableHarnessProbe::_unverifiedExpectation()
{
    expectLogMessage("Test.PortableHarnessProbe", QtWarningMsg, QRegularExpression(QStringLiteral("never verified")));
}

// Fails: the expected message was not logged
void PortableHarnessProbe::_expectationWithoutMessage()
{
    expectLogMessage("Test.PortableHarnessProbe", QtWarningMsg, QRegularExpression(QStringLiteral("never logged")));
    verifyExpectedLogMessage();
}

// Passes: no message is captured, as debug and info messages are off
void PortableHarnessProbe::_debugAndInfoAreOff()
{
    qCDebug(PortableHarnessProbeLog) << "a debug message";
    qCInfo(PortableHarnessProbeLog) << "an info message";
}

// Passes: a message is captured on whichever thread logs it
void PortableHarnessProbe::_warningFromAnotherThread()
{
    expectLogMessage("Test.PortableHarnessProbe", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^from another thread$")));
    const std::unique_ptr<QThread> thread(
        QThread::create([]() { qCWarning(PortableHarnessProbeLog) << "from another thread"; }));
    thread->start();
    QVERIFY(thread->wait(TestTimeout::mediumMs()));
    verifyExpectedLogMessage();
}

// Runs the one function named: some fail on purpose
int main(int argc, char* argv[])
{
    const QCoreApplication application(argc, argv);
    if ((argc != 2) || (argv[1][0] != '_')) {
        (void) std::fputs("usage: PortableHarnessProbe <test function>\n", stderr);
        return 2;
    }

    PortableHarnessProbe probe;
    return QTest::qExec(&probe, argc, argv);
}

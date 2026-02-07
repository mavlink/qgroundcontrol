#include "FTPTest.h"

#include <QtTest/QSignalSpy>

#include "FTPManager.h"
#include <QtCore/QLoggingCategory>
#include "Vehicle.h"

Q_STATIC_LOGGING_CATEGORY(FTPTestLog, "Test.FTPTest")

FTPTest::FTPTest(QObject* parent) : VehicleTest(parent)
{
    setWaitForInitialConnect(true);
}

void FTPTest::init()
{
    VehicleTest::init();

    if (!ftpManager()) {
        qCWarning(FTPTestLog) << "FTPTest::init - FTPManager not available";
    }
}

FTPManager* FTPTest::ftpManager() const
{
    return _vehicle ? _vehicle->ftpManager() : nullptr;
}

bool FTPTest::waitForFTPComplete(int timeoutMs)
{
    if (!ftpManager()) {
        return false;
    }

    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    // Wait for commandProgress to reach 1.0 (complete)
    QSignalSpy spy(ftpManager(), &FTPManager::commandProgress);
    while (spy.wait(timeoutMs)) {
        if (!spy.isEmpty()) {
            float progress = spy.last().at(0).toFloat();
            if (progress >= 1.0f) {
                return true;
            }
        }
    }
    return false;
}

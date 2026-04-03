/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4SITLTestBase.h"

#include "Vehicle.h"

#include <QtCore/QElapsedTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

Q_LOGGING_CATEGORY(PX4SITLTestLog, "qgc.test.sitl.px4")

bool PX4SITLTestBase::armVehicle(int timeoutMs)
{
    if (!vehicle()) return false;

    // PX4 may temporarily reject arming while preflight checks are still running.
    // Retry the arm command periodically until it succeeds or we time out.
    QElapsedTimer timer;
    timer.start();

    while (!timer.hasExpired(timeoutMs)) {
        vehicle()->setArmed(true, false);

        // Wait up to 3 seconds for this attempt
        const int attemptTimeout = qMin(3000, timeoutMs - static_cast<int>(timer.elapsed()));
        if (attemptTimeout <= 0) break;

        if (waitForArmedState(true, attemptTimeout)) {
            return true;
        }

        // Not armed yet — PX4 probably rejected. Wait a bit before retrying.
        QTest::qWait(1000);
    }

    qCWarning(PX4SITLTestLog) << "Failed to arm after" << timer.elapsed() << "ms";
    return false;
}

bool PX4SITLTestBase::disarmVehicle(int timeoutMs)
{
    if (!vehicle()) return false;

    vehicle()->setArmed(false, false);
    return waitForArmedState(false, timeoutMs);
}

bool PX4SITLTestBase::takeoff(float altitudeM, int timeoutMs)
{
    if (!vehicle()) return false;

    // Use the takeoff flight mode — PX4 handles altitude via MIS_TAKEOFF_ALT
    // or we can set it via the takeoff action
    vehicle()->guidedModeTakeoff(altitudeM);
    return waitForAltitude(altitudeM, 2.0f, timeoutMs);
}

bool PX4SITLTestBase::land(int timeoutMs)
{
    if (!vehicle()) return false;

    vehicle()->guidedModeLand();
    return waitForAltitude(0.0f, 1.0f, timeoutMs);
}

bool PX4SITLTestBase::setFlightMode(const QString &mode, int timeoutMs)
{
    if (!vehicle()) return false;

    vehicle()->setFlightMode(mode);
    return waitForCondition(
        [this, &mode]() { return vehicle()->flightMode() == mode; },
        timeoutMs,
        QStringLiteral("flightMode == %1").arg(mode));
}

bool PX4SITLTestBase::waitForAltitude(float targetM, float toleranceM, int timeoutMs)
{
    return waitForCondition(
        [this, targetM, toleranceM]() {
            if (!vehicle()) return false;
            const float alt = vehicle()->altitudeRelative()->rawValue().toFloat();
            return qAbs(alt - targetM) <= toleranceM;
        },
        timeoutMs,
        QStringLiteral("altitude ≈ %1m ±%2m").arg(targetM).arg(toleranceM));
}

bool PX4SITLTestBase::waitForArmedState(bool armed, int timeoutMs)
{
    return waitForCondition(
        [this, armed]() { return vehicle() && vehicle()->armed() == armed; },
        timeoutMs,
        QStringLiteral("armed == %1").arg(armed));
}

/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "tst_MAVLinkStandardModes.h"

#include "FirmwarePlugin.h"
#include "Vehicle.h"

#include <QtCore/QLoggingCategory>
#include <QtTest/QTest>

Q_DECLARE_LOGGING_CATEGORY(SITLTestLog)

void SITLStandardModesTest::testAvailableModes()
{
    QVERIFY(vehicle());

    // StandardModes are requested during InitialConnectStateMachine.
    // After init(), the firmware plugin should have a populated mode list.
    const QStringList modes = vehicle()->flightModes();
    QVERIFY2(!modes.isEmpty(), "No flight modes reported — AVAILABLE_MODES may not have been received");

    qCInfo(SITLTestLog) << "Available flight modes (" << modes.count() << "):";
    for (const QString &mode : modes) {
        qCInfo(SITLTestLog) << "  -" << mode;
    }

    // Verify essential standard modes are present (PX4 SIH should always report these)
    const QStringList expectedModes = {
        QStringLiteral("Mission"),
        QStringLiteral("Land"),
        QStringLiteral("Takeoff"),
    };

    for (const QString &expected : expectedModes) {
        QVERIFY2(modes.contains(expected),
                 qPrintable(QStringLiteral("Expected mode '%1' not found in available modes").arg(expected)));
    }
}

UT_REGISTER_TEST(SITLStandardModesTest, TestLabel::SITL, TestLabel::MAVLinkProtocol)

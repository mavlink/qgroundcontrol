/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "tst_MAVLinkParamSync.h"

#include "ParameterManager.h"
#include "Vehicle.h"

#include <QtCore/QLoggingCategory>
#include <QtTest/QTest>

Q_DECLARE_LOGGING_CATEGORY(SITLTestLog)

void SITLParamSyncTest::testFullDownload()
{
    QVERIFY(vehicle());
    ParameterManager *pm = vehicle()->parameterManager();
    QVERIFY(pm);

    // After init(), parameters should be fully synced
    QVERIFY2(pm->parametersReady(), "Parameters not ready after initial connect");

    // Verify we can read a known PX4 parameter
    const int compId = ParameterManager::defaultComponentId;
    QVERIFY2(pm->parameterExists(compId, QStringLiteral("SYS_AUTOSTART")),
             "SYS_AUTOSTART parameter not found — parameter sync may be incomplete");

    qCInfo(SITLTestLog) << "Parameter sync complete, parametersReady=true";
}

void SITLParamSyncTest::testModifyRoundTrip()
{
    QVERIFY(vehicle());
    ParameterManager *pm = vehicle()->parameterManager();
    QVERIFY(pm);
    QVERIFY(pm->parametersReady());

    // Use a safe, non-critical parameter for testing
    const int compId = ParameterManager::defaultComponentId;
    const QString paramName = QStringLiteral("MPC_Z_VEL_MAX_UP");

    QVERIFY2(pm->parameterExists(compId, paramName),
             qPrintable(QStringLiteral("Parameter %1 not found").arg(paramName)));

    Fact *param = pm->getParameter(compId, paramName);
    QVERIFY(param);

    const QVariant originalValue = param->rawValue();
    const float original = originalValue.toFloat();
    const float modified = original + 0.5f;

    // Write modified value
    param->setRawValue(modified);

    // Wait for the parameter to be sent and acknowledged
    QVERIFY(waitForCondition(
        [param, modified]() { return qFuzzyCompare(param->rawValue().toFloat(), modified); },
        TestTimeout::mediumMs(),
        QStringLiteral("param == modified")));

    // Restore original value and wait for PX4 to acknowledge
    param->setRawValue(original);
    QVERIFY(waitForCondition(
        [param, original]() { return qFuzzyCompare(param->rawValue().toFloat(), original); },
        TestTimeout::mediumMs(),
        QStringLiteral("param == original")));

    // Let any pending PARAM_SET retransmissions settle before cleanup kills the container
    QTest::qWait(500);

    qCInfo(SITLTestLog) << "Parameter round-trip verified:" << paramName
                       << original << "→" << modified << "→" << original;
}

UT_REGISTER_TEST(SITLParamSyncTest, TestLabel::SITL, TestLabel::MAVLinkProtocol)

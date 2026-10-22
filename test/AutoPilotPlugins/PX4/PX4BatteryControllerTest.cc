#include "PX4BatteryControllerTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "PX4BatteryController.h"
#include "ParameterManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PX4BatteryControllerTest, TestLabel::Integration, TestLabel::Vehicle)

void PX4BatteryControllerTest::_requestedParamPendingUntilReadEnds_data()
{
    QTest::addColumn<QString>("paramName");
    QTest::addColumn<bool>("initiallySent");
    QTest::addColumn<bool>("inFirmware");
    QTest::addColumn<int>("vehicleValue");

    QTest::newRow("sentInInitialDownload") << QStringLiteral("BAT1_SOURCE") << true << true << 0;
    // A board with one power input never looks up battery 2
    QTest::newRow("unusedAnsweredByName") << QStringLiteral("BAT2_SOURCE") << false << true << 1;
    QTest::newRow("notInFirmware") << QStringLiteral("BAT9_SOURCE") << false << false << 0;
}

void PX4BatteryControllerTest::_requestedParamPendingUntilReadEnds()
{
    QFETCH(QString, paramName);
    QFETCH(bool, initiallySent);
    QFETCH(bool, inFirmware);
    QFETCH(int, vehicleValue);

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    const auto exists = [parameterManager, &paramName]() {
        return parameterManager->parameterExists(ParameterManager::defaultComponentId, paramName);
    };
    QCOMPARE(exists(), initiallySent);

    PX4BatteryController controller;
    QVariant valueWhenAdded;
    bool readyWhenAdded = false;
    int addedCountWhenNoLongerPending = -1;
    (void) connect(&controller, &PX4BatteryController::addedParamCountChanged, this, [&]() {
        valueWhenAdded = parameterManager->getParameter(ParameterManager::defaultComponentId, paramName)->rawValue();
        readyWhenAdded = controller.parameterReady(paramName);
    });
    (void) connect(&controller, &PX4BatteryController::pendingParamsChanged, this, [&]() {
        if (!controller.pendingParams().contains(paramName)) {
            addedCountWhenNoLongerPending = controller.addedParamCount();
        }
    });

    controller.requestParameters({paramName});
    QCOMPARE(controller.pendingParams().contains(paramName), !initiallySent);
    QCOMPARE(controller.parameterReady(paramName), initiallySent);

    // Strict log checking fails the test if a read failure is reported to the user
    QVERIFY(UnitTest::waitForCondition([&controller]() { return controller.pendingParams().isEmpty(); },
                                       TestTimeout::mediumMs(), QStringLiteral("requested parameters read")));
    QCOMPARE(exists(), inFirmware);
    QCOMPARE(controller.parameterReady(paramName), inFirmware);

    const bool arrived = inFirmware && !initiallySent;
    QCOMPARE(controller.addedParamCount(), arrived ? 1 : 0);
    if (!initiallySent) {
        // The page rebuilds its rows when addedParamCount changes, and picks its filter when pendingParams changes
        // using those rows, so the count must change first
        QCOMPARE(addedCountWhenNoLongerPending, arrived ? 1 : 0);
    }
    if (arrived) {
        // The rows rebuilt on addedParamCount show the vehicle's value
        QCOMPARE(valueWhenAdded.toInt(), vehicleValue);
        QVERIFY(readyWhenAdded);
    }
}

// Reopening the page does not wait for names the vehicle did not answer the first time
void PX4BatteryControllerTest::_unansweredParamNotRequestedAgain()
{
    const QString paramName = QStringLiteral("BAT9_SOURCE");
    {
        PX4BatteryController controller;
        controller.requestParameters({paramName});
        QVERIFY(controller.pendingParams().contains(paramName));
        QVERIFY(UnitTest::waitForCondition([&controller]() { return controller.pendingParams().isEmpty(); },
                                           TestTimeout::mediumMs(), QStringLiteral("requested parameters read")));
    }

    {
        PX4BatteryController controller;
        controller.requestParameters({paramName});
        QVERIFY(controller.pendingParams().isEmpty());
        QVERIFY(!controller.parameterReady(paramName));
    }

    // A reconnected vehicle is asked again
    _disconnectMockLink();
    _connectMockLink();
    QVERIFY(waitForInitialConnect());
    QVERIFY(waitForParametersReady());
    PX4BatteryController controller;
    controller.requestParameters({paramName});
    QVERIFY(controller.pendingParams().contains(paramName));
    QVERIFY(UnitTest::waitForCondition([&controller]() { return controller.pendingParams().isEmpty(); },
                                       TestTimeout::mediumMs(), QStringLiteral("requested parameters read")));
}

// Parameters the vehicle adds while the page is open, such as after another page requests them, also update it
void PX4BatteryControllerTest::_unrequestedParamCounted()
{
    PX4BatteryController controller;
    QSignalSpy addedSpy(&controller, &PX4BatteryController::addedParamCountChanged);
    QVERIFY(addedSpy.isValid());

    ParameterManager* const parameterManager = vehicle()->parameterManager();
    const QString paramName = QStringLiteral("BAT2_SOURCE");
    QVERIFY(!parameterManager->parameterExists(ParameterManager::defaultComponentId, paramName));
    parameterManager->refreshParameter(ParameterManager::defaultComponentId, paramName, false /* notifyFailure */);

    QVERIFY(addedSpy.wait(TestTimeout::mediumMs()));
    QCOMPARE(controller.addedParamCount(), 1);
    QVERIFY(controller.parameterReady(paramName));
    QCOMPARE(parameterManager->getParameter(ParameterManager::defaultComponentId, paramName)->rawValue().toInt(), 1);
    QVERIFY(controller.pendingParams().isEmpty());
}

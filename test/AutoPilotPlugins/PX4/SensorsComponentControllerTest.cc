#include "SensorsComponentControllerTest.h"

#include "SensorsComponentController.h"
#include "Vehicle.h"

UT_REGISTER_TEST_LIGHTWEIGHT(SensorsComponentControllerTest, TestLabel::Unit)

void SensorsComponentControllerTest::_largeVehicleCalibrationResult_data()
{
    using Result = SensorsComponentController::LargeVehicleCalibrationResult;

    QTest::addColumn<QString>("text");
    QTest::addColumn<Result>("expected");

    QTest::newRow("magFinished") << QStringLiteral("Mag quick calibration finished") << Result::Succeeded;
    QTest::newRow("magNeedsGps") << QStringLiteral("GPS required for mag quick cal") << Result::Failed;
    QTest::newRow("magNeedsAttitude") << QStringLiteral("attitude required for mag quick cal") << Result::Failed;
    QTest::newRow("saveFailed") << QStringLiteral("calibration save failed") << Result::Failed;
    QTest::newRow("storageFailed") << QStringLiteral("ERROR: cannot open calibration storage") << Result::Failed;
    QTest::newRow("magFailed") << QStringLiteral("[cal] calibration failed: mag") << Result::Failed;
    // Progress text, such as the heading the firmware assumes, says nothing about the outcome
    QTest::newRow("assumedHeading") << QStringLiteral("Assuming vehicle is facing heading 90.0 degrees")
                                    << Result::Pending;
    QTest::newRow("unrelated") << QStringLiteral("Armed by internal command") << Result::Pending;
}

void SensorsComponentControllerTest::_largeVehicleCalibrationResult()
{
    using Result = SensorsComponentController::LargeVehicleCalibrationResult;

    QFETCH(QString, text);
    QFETCH(Result, expected);

    QCOMPARE(SensorsComponentController::largeVehicleCalibrationResult(text), expected);
}

void SensorsComponentControllerTest::_largeVehicleCommandFailureText_data()
{
    QTest::addColumn<int>("failureCode");
    QTest::addColumn<QString>("expected");

    QTest::newRow("noResponse") << static_cast<int>(Vehicle::MavCmdResultFailureNoResponseToCommand)
                                << QStringLiteral("The vehicle did not respond.");
    QTest::newRow("duplicate") << static_cast<int>(Vehicle::MavCmdResultFailureDuplicateCommand)
                               << QStringLiteral("A calibration request is already waiting for the vehicle.");
    // The vehicle answered and refused, such as when armed
    QTest::newRow("refused")
        << static_cast<int>(Vehicle::MavCmdResultCommandResultOnly)
        << QStringLiteral(
               "The vehicle did not accept the calibration. It may be armed or busy, or its firmware may not "
               "support it.");
}

void SensorsComponentControllerTest::_largeVehicleCommandFailureText()
{
    QFETCH(int, failureCode);
    QFETCH(QString, expected);

    QCOMPARE(SensorsComponentController::largeVehicleCommandFailureText(failureCode), expected);
}

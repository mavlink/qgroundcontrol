#include "RCToParamDialogControllerTest.h"

#include <QtCore/QRegularExpression>

#include "Fact.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "RCToParamDialogController.h"
#include "Vehicle.h"

void RCToParamDialogControllerTest::_setTuningFactWithoutVehicle()
{
    QVERIFY(!MultiVehicleManager::instance()->activeVehicle());

    Fact tuningFact(0, QStringLiteral("TEST_PARAM"), FactMetaData::valueTypeDouble);
    tuningFact.metaData()->setRawMin(-5.0);
    tuningFact.metaData()->setRawMax(10.0);
    tuningFact.setRawValue(2.5);

    RCToParamDialogController controller;
    expectLogMessage("QMLControls.RCToParamDialogController", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No active vehicle to refresh")));
    controller.setTuningFact(&tuningFact);
    verifyExpectedLogMessage();

    QCOMPARE(controller.tuningFact(), &tuningFact);
    QCOMPARE(controller.center()->rawValue().toDouble(), 2.5);
    QCOMPARE(controller.min()->rawValue().toDouble(), -5.0);
    QCOMPARE(controller.max()->rawValue().toDouble(), 10.0);
    QVERIFY(!controller.property("ready").toBool());

    emit tuningFact.vehicleUpdated(QVariant(3.0));
    QVERIFY(controller.property("ready").toBool());
}

void RCToParamDialogControllerVehicleTest::_setTuningFactRefreshesParameter()
{
    QVERIFY(vehicle());
    Fact* const tuningFact =
        vehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId, QStringLiteral("MC_ROLL_P"));

    RCToParamDialogController controller;
    controller.setTuningFact(tuningFact);
    QCOMPARE(controller.center()->rawValue().toDouble(), tuningFact->rawValue().toDouble());

    QTRY_VERIFY_WITH_TIMEOUT(controller.property("ready").toBool(), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(RCToParamDialogControllerTest, TestLabel::Unit)
UT_REGISTER_TEST(RCToParamDialogControllerVehicleTest, TestLabel::Integration, TestLabel::Vehicle)

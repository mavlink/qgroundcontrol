#include "FactSystemTestBase.h"

#include "AutoPilotPlugin.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"

/// FactSystem Unit Test

void FactSystemTestBase::_init(MAV_AUTOPILOT autopilot)
{
    // UnitTest::init() is called by the derived test class before this
    MultiVehicleManager::instance()->init();

    _connectMockLink(autopilot);

    _plugin = MultiVehicleManager::instance()->activeVehicle()->autopilotPlugin();
    QVERIFY(_plugin);
}

void FactSystemTestBase::_cleanup()
{
    _disconnectMockLink();
    UnitTest::cleanup();
}

/// Basic test of parameter values in Fact System
void FactSystemTestBase::_parameter_default_component_id_test()
{
    QVERIFY(_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "BAT1_N_CELLS"));
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "BAT1_N_CELLS");
    QVERIFY(fact != nullptr);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);

    QCOMPARE(factValue.toInt(), 4);
}

void FactSystemTestBase::_parameter_specific_component_id_test()
{
    QVERIFY(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1, "BAT1_N_CELLS"));
    Fact* fact = _vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "BAT1_N_CELLS");
    QVERIFY(fact != nullptr);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);
    QCOMPARE(factValue.toInt(), 4);
}


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
    QVERIFY(_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE"));
    Fact* fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE");
    QVERIFY(fact != nullptr);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);

    QCOMPARE(factValue.toInt(), 3);
}

void FactSystemTestBase::_parameter_specific_component_id_test()
{
    QVERIFY(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE"));
    Fact* fact = _vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE");
    QVERIFY(fact != nullptr);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);
    QCOMPARE(factValue.toInt(), 3);
}

void FactSystemTestBase::_qml_test()
{
    QSKIP("Requires QML test infrastructure (QGCQuickWidget removed in Qt6 migration)");
}

void FactSystemTestBase::_qmlUpdate_test()
{
    QSKIP("Requires QML test infrastructure (QGCQuickWidget removed in Qt6 migration)");
}

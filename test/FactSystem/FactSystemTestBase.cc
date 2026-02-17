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
    Q_ASSERT(_plugin);
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

/// Test that QML can reference a Fact
void FactSystemTestBase::_qml_test()
{
    //-- TODO
#if 0
    QGCQuickWidget* widget = new QGCQuickWidget;

    widget->setAutoPilot(_plugin);

    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));

    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QVariant qmlValue = control->property("text").toInt();

    QCOMPARE(qmlValue.toInt(), 3);

    delete widget;
#endif
}

/// Test QML getting an updated Fact value
void FactSystemTestBase::_qmlUpdate_test()
{
    //-- TODO
#if 0
    QGCQuickWidget* widget = new QGCQuickWidget;

    widget->setAutoPilot(_plugin);

    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));

    // Change the value

    QVariant paramValue = 12;
    MultiVehicleManager::instance()->activeVehicle()->parameterManager()->getParameter(ParameterManager::defaultComponentId, "RC_MAP_THROTTLE")->setRawValue(paramValue);

    QTest::qWait(500); // Let the signals flow through

    // Make sure the qml has the right value

    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QCOMPARE(control->property("text").toInt(), 12);

    delete widget;
#endif
}

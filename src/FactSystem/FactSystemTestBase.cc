/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystemTestBase.h"
#include "LinkManager.h"
#ifdef QT_DEBUG
#include "MockLink.h"
#endif
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "ParameterManager.h"

#include <QQuickItem>

/// FactSystem Unit Test
FactSystemTestBase::FactSystemTestBase(void)
{

}

void FactSystemTestBase::_init(MAV_AUTOPILOT autopilot)
{
    UnitTest::init();

    _connectMockLink(autopilot);

    _plugin = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->autopilotPlugin();
    Q_ASSERT(_plugin);
}

void FactSystemTestBase::_cleanup(void)
{
    UnitTest::cleanup();
}

/// Basic test of parameter values in Fact System
void FactSystemTestBase::_parameter_default_component_id_test(void)
{
    QVERIFY(_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, "RC_MAP_THROTTLE"));
    Fact* fact = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, "RC_MAP_THROTTLE");
    QVERIFY(fact != nullptr);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);

    QCOMPARE(factValue.toInt(), 3);
}

void FactSystemTestBase::_parameter_specific_component_id_test(void)
{
    QVERIFY(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE"));
    Fact* fact = _vehicle->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "RC_MAP_THROTTLE");
    QVERIFY(fact != nullptr);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);
    QCOMPARE(factValue.toInt(), 3);
}

/// Test that QML can reference a Fact
void FactSystemTestBase::_qml_test(void)
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
void FactSystemTestBase::_qmlUpdate_test(void)
{
    //-- TODO
#if 0
    QGCQuickWidget* widget = new QGCQuickWidget;

    widget->setAutoPilot(_plugin);

    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));

    // Change the value

    QVariant paramValue = 12;
    qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->parameterManager()->getParameter(FactSystem::defaultComponentId, "RC_MAP_THROTTLE")->setRawValue(paramValue);

    QTest::qWait(500); // Let the signals flow through

    // Make sure the qml has the right value

    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QCOMPARE(control->property("text").toInt(), 12);

    delete widget;
#endif
}


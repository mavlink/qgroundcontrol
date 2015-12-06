/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystemTestBase.h"
#include "LinkManager.h"
#ifdef QT_DEBUG
#include "MockLink.h"
#endif
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCQuickWidget.h"

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
    QVERIFY(_plugin->factExists(FactSystem::ParameterProvider, FactSystem::defaultComponentId, "RC_MAP_THROTTLE"));
    Fact* fact = _plugin->getFact(FactSystem::ParameterProvider, FactSystem::defaultComponentId, "RC_MAP_THROTTLE");
    QVERIFY(fact != NULL);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);

    QCOMPARE(factValue.toInt(), 3);
}

void FactSystemTestBase::_parameter_specific_component_id_test(void)
{
    QVERIFY(_plugin->factExists(FactSystem::ParameterProvider, 50, "RC_MAP_THROTTLE"));
    Fact* fact = _plugin->getFact(FactSystem::ParameterProvider, 50, "RC_MAP_THROTTLE");
    QVERIFY(fact != NULL);
    QVariant factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);


    QCOMPARE(factValue.toInt(), 3);

    // Test another component id
    QVERIFY(_plugin->factExists(FactSystem::ParameterProvider, 51, "COMPONENT_51"));
    fact = _plugin->getFact(FactSystem::ParameterProvider, 51, "COMPONENT_51");
    QVERIFY(fact != NULL);
    factValue = fact->rawValue();
    QCOMPARE(factValue.isValid(), true);

    QCOMPARE(factValue.toInt(), 51);
}

/// Test that QML can reference a Fact
void FactSystemTestBase::_qml_test(void)
{
    QGCQuickWidget* widget = new QGCQuickWidget;

    widget->setAutoPilot(_plugin);

    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));

    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QVariant qmlValue = control->property("text").toInt();

    QCOMPARE(qmlValue.toInt(), 3);

    delete widget;
}

/// Test QML getting an updated Fact value
void FactSystemTestBase::_qmlUpdate_test(void)
{
    QGCQuickWidget* widget = new QGCQuickWidget;

    widget->setAutoPilot(_plugin);

    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));

    // Change the value

    QVariant paramValue = 12;
    _plugin->getParameterFact(FactSystem::defaultComponentId, "RC_MAP_THROTTLE")->setRawValue(paramValue);

    QTest::qWait(500); // Let the signals flow through

    // Make sure the qml has the right value

    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QCOMPARE(control->property("text").toInt(), 12);

    delete widget;
}


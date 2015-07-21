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
#include "MockLink.h"
#include "AutoPilotPluginManager.h"
#include "UASManager.h"
#include "QGCApplication.h"
#include "QGCMessageBox.h"
#include "QGCQuickWidget.h"

#include <QQuickItem>

/// FactSystem Unit Test
FactSystemTestBase::FactSystemTestBase(void)
{
    
}

void FactSystemTestBase::_init(MAV_AUTOPILOT autopilot)
{
    UnitTest::init();
    
    LinkManager* _linkMgr = LinkManager::instance();
    
    MockLink* link = new MockLink();
    link->setAutopilotType(autopilot);
    _linkMgr->_addLink(link);
    
    if (autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        // Connect will pop a warning dialog
        setExpectedMessageBox(QGCMessageBox::Ok);
    }
    _linkMgr->connectLink(link);
    
    // Wait for the uas to work it's way through the various threads

    QSignalSpy spyUas(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)));
    QCOMPARE(spyUas.wait(5000), true);
    
    if (autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        checkExpectedMessageBox();
    }
    
    _uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(_uas);
    
    // Get the plugin for the uas
    
    AutoPilotPluginManager* pluginMgr = AutoPilotPluginManager::instance();
    Q_ASSERT(pluginMgr);
    
    _plugin = pluginMgr->getInstanceForAutoPilotPlugin(_uas).data();
    Q_ASSERT(_plugin);

    // Wait for the plugin to be ready
    
    QSignalSpy spyPlugin(_plugin, SIGNAL(pluginReadyChanged(bool)));
    if (!_plugin->pluginReady()) {
        QCOMPARE(spyPlugin.wait(60000), true);
    }
    Q_ASSERT(_plugin->pluginReady());
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
    QVariant factValue = fact->value();
    QCOMPARE(factValue.isValid(), true);
    
    QCOMPARE(factValue.toInt(), 3);
}

void FactSystemTestBase::_parameter_specific_component_id_test(void)
{
    QVERIFY(_plugin->factExists(FactSystem::ParameterProvider, 50, "RC_MAP_THROTTLE"));
    Fact* fact = _plugin->getFact(FactSystem::ParameterProvider, 50, "RC_MAP_THROTTLE");
    QVERIFY(fact != NULL);
    QVariant factValue = fact->value();
    QCOMPARE(factValue.isValid(), true);
    
    
    QCOMPARE(factValue.toInt(), 3);
    
    // Test another component id
    QVERIFY(_plugin->factExists(FactSystem::ParameterProvider, 51, "COMPONENT_51"));
    fact = _plugin->getFact(FactSystem::ParameterProvider, 51, "COMPONENT_51");
    QVERIFY(fact != NULL);
    factValue = fact->value();
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
    _plugin->getParameterFact(FactSystem::defaultComponentId, "RC_MAP_THROTTLE")->setValue(paramValue);

    QTest::qWait(500); // Let the signals flow through
    
    // Make sure the qml has the right value
    
    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QCOMPARE(control->property("text").toInt(), 12);
    
    delete widget;
}


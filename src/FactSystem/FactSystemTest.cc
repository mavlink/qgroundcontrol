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

#include "FactSystemTest.h"
#include "LinkManager.h"
#include "MockLink.h"
#include "AutoPilotPluginManager.h"
#include "UASManager.h"
#include "QGCApplication.h"
#include "QGCQuickWidget.h"

#include <QQuickItem>

UT_REGISTER_TEST(FactSystemTest)

/// FactSystem Unit Test
FactSystemTest::FactSystemTest(void)
{
    
}

void FactSystemTest::init(void)
{
    UnitTest::init();
    
    LinkManager* _linkMgr = LinkManager::instance();
    
    MockLink* link = new MockLink();
    _linkMgr->addLink(link);
    _linkMgr->connectLink(link);
    
    // Wait for the uas to work it's way through the various threads

    QSignalSpy spyUas(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)));
    QCOMPARE(spyUas.wait(5000), true);
    
    _uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(_uas);
    
    _paramMgr = _uas->getParamManager();
    Q_ASSERT(_paramMgr);
    
    // Get the plugin for the uas
    
    AutoPilotPluginManager* pluginMgr = AutoPilotPluginManager::instance();
    Q_ASSERT(pluginMgr);
    
    _plugin = pluginMgr->getInstanceForAutoPilotPlugin(_uas);
    Q_ASSERT(_plugin);

    // Wait for the plugin to be ready
    
    QSignalSpy spyPlugin(_plugin, SIGNAL(pluginReady()));
    if (!_plugin->pluginIsReady()) {
        QCOMPARE(spyPlugin.wait(5000), true);
    }
    Q_ASSERT(_plugin->pluginIsReady());
}

void FactSystemTest::cleanup(void)
{
    UnitTest::cleanup();
}

/// Basic test of parameter values in Fact System
void FactSystemTest::_parameter_test(void)
{
    // Get the parameter facts from the AutoPilot
    
    AutoPilotPluginManager* pluginMgr = AutoPilotPluginManager::instance();
    Q_ASSERT(pluginMgr);
    
    AutoPilotPlugin* plugin = pluginMgr->getInstanceForAutoPilotPlugin(_uas);
    Q_ASSERT(plugin);
    
    const QVariantMap& parameterFacts = plugin->parameterFacts();
    
    // Compare the value in the Parameter Manager with the value from the FactSystem
    
    Fact* fact = parameterFacts["RC_MAP_THROTTLE"].value<Fact*>();
    QVERIFY(fact != NULL);
    QVariant factValue = fact->value();
    QCOMPARE(factValue.isValid(), true);
    
    QVariant paramValue;
    Q_ASSERT(_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), "RC_MAP_THROTTLE", paramValue));
    
    QCOMPARE(factValue.toInt(), paramValue.toInt());
}

/// Test that QML can reference a Fact
void FactSystemTest::_qml_test(void)
{
    QGCQuickWidget* widget = new QGCQuickWidget;
    
    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));
    
    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QVariant qmlValue = control->property("text").toInt();

    QVariant paramMgrValue;
    Q_ASSERT(_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), "RC_MAP_THROTTLE", paramMgrValue));
    
    QCOMPARE(qmlValue.toInt(), paramMgrValue.toInt());
}

// Test correct behavior when the Param Manager gets a parameter update
void FactSystemTest::_paramMgrSignal_test(void)
{
    // Get the parameter Fact from the AutoPilot
    
    AutoPilotPluginManager* pluginMgr = AutoPilotPluginManager::instance();
    Q_ASSERT(pluginMgr);
    
    AutoPilotPlugin* plugin = pluginMgr->getInstanceForAutoPilotPlugin(_uas);
    Q_ASSERT(plugin);
    
    const QVariantMap& parameterFacts = plugin->parameterFacts();
    
    Fact* fact = parameterFacts["RC_MAP_THROTTLE"].value<Fact*>();
    QVERIFY(fact != NULL);
    
    // Setting a new value into the parameter should trigger a valueChanged signal on the Fact

    QSignalSpy spyFact(fact, SIGNAL(valueChanged(QVariant)));
    
    QVariant paramValue = 12;
    _paramMgr->setParameter(_paramMgr->getDefaultComponentId(), "RC_MAP_THROTTLE", paramValue);
    _paramMgr->sendPendingParameters(true, false);

    // Wait for the Fact::valueChanged signal to come through
    QCOMPARE(spyFact.wait(5000), true);

    // Make sure the signal has the right value
    QList<QVariant> arguments = spyFact.takeFirst();
    qDebug() << arguments.at(0).type();
    QCOMPARE(arguments.at(0).toInt(), 12);
    
    // Make sure the Fact has the new value
    QCOMPARE(fact->value().toInt(), 12);
}

/// Test QML getting an updated Fact value
void FactSystemTest::_qmlUpdate_test(void)
{
    QGCQuickWidget* widget = new QGCQuickWidget;
    
    widget->setSource(QUrl::fromUserInput("qrc:unittest/FactSystemTest.qml"));
    
    // Change the value using param manager
    
    QVariant paramValue = 12;
    _paramMgr->setParameter(_paramMgr->getDefaultComponentId(), "RC_MAP_THROTTLE", paramValue);
    _paramMgr->sendPendingParameters(true, false);

    QTest::qWait(500); // Let the signals flow through
    
    // Make sure the qml has the right value
    
    QQuickItem* rootObject = widget->rootObject();
    QObject* control = rootObject->findChild<QObject*>("testControl");
    QVERIFY(control != NULL);
    QCOMPARE(control->property("text").toInt(), 12);
}


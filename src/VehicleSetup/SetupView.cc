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

#include "SetupView.h"
#include "ui_SetupView.h"

#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "VehicleComponent.h"
#include "QGCQmlWidgetHolder.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"
#ifndef __android__
#include "FirmwareUpgradeController.h"
#endif
#include "ParameterEditorController.h"

#include <QQmlError>
#include <QQmlContext>
#include <QDebug>

SetupView::SetupView(QWidget* parent) :
    QWidget(parent),
    _uasCurrent(NULL),
    _initComplete(false),
    _autoPilotPlugin(NULL),
    _currentSetupWidget(NULL),
    _ui(new Ui::SetupView)
{
    _ui->setupUi(this);

    bool fSucceeded = connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    Q_UNUSED(fSucceeded);
    Q_ASSERT(fSucceeded);
    
#ifndef __android__
    qmlRegisterType<FirmwareUpgradeController>("QGroundControl.Controllers", 1, 0, "FirmwareUpgradeController");
#endif
	
    _ui->buttonHolder->rootContext()->setContextProperty("controller", this);
    _ui->buttonHolder->setAutoPilot(NULL);
    _ui->buttonHolder->setSource(QUrl::fromUserInput("qrc:/qml/SetupViewButtonsDisconnected.qml"));
    
    _setActiveUAS(UASManager::instance()->getActiveUAS());
}

SetupView::~SetupView()
{

}

void SetupView::_setActiveUAS(UASInterface* uas)
{
    if (_uasCurrent) {
        Q_ASSERT(_autoPilotPlugin);
        disconnect(_autoPilotPlugin, &AutoPilotPlugin::pluginReadyChanged, this, &SetupView::_pluginReadyChanged);
    }

    _pluginReadyChanged(false);
    
    _uasCurrent = uas;
    
    if (_uasCurrent) {
        _autoPilotPlugin = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uasCurrent);
        _pluginReadyChanged(_autoPilotPlugin->pluginReady());
        connect(_autoPilotPlugin, &AutoPilotPlugin::pluginReadyChanged, this, &SetupView::_pluginReadyChanged);
    }
}

void SetupView::_pluginReadyChanged(bool pluginReady)
{
    if (pluginReady) {
        _ui->buttonHolder->setAutoPilot(_autoPilotPlugin);
        _ui->buttonHolder->setSource(QUrl::fromUserInput("qrc:/qml/SetupViewButtonsConnected.qml"));
        summaryButtonClicked();
        QObject* button = _ui->buttonHolder->rootObject()->findChild<QObject*>("summaryButton");
        Q_ASSERT(button);
        button->setProperty("checked", true);
    } else {
        _ui->buttonHolder->setSource(QUrl::fromUserInput("qrc:/qml/SetupViewButtonsDisconnected.qml"));
        _ui->buttonHolder->setAutoPilot(NULL);
        firmwareButtonClicked();
        QObject* button = _ui->buttonHolder->rootObject()->findChild<QObject*>("firmwareButton");
        Q_ASSERT(button);
        button->setProperty("checked", true);
    }
}

void SetupView::_changeSetupWidget(QWidget* newWidget)
{
    if (_currentSetupWidget) {
        delete _currentSetupWidget;
    }
    _currentSetupWidget = newWidget;
    _ui->horizontalLayout->addWidget(newWidget);
}

void SetupView::firmwareButtonClicked(void)
{
#ifndef __android__
    //FIXME: Hack out for android for now
    if (_uasCurrent && _uasCurrent->isArmed()) {
        QGCMessageBox::warning("Setup", "Firmware Update cannot be performed while vehicle is armed.");
        return;
    }

    QGCQmlWidgetHolder* setup = new QGCQmlWidgetHolder;
    Q_CHECK_PTR(setup);
    
    setup->setSource(QUrl::fromUserInput("qrc:/qml/FirmwareUpgrade.qml"));

    _changeSetupWidget(setup);
#endif
}

void SetupView::parametersButtonClicked(void)
{
	QGCQmlWidgetHolder* setup = new QGCQmlWidgetHolder;
	Q_CHECK_PTR(setup);

	Q_ASSERT(_autoPilotPlugin);
	setup->setAutoPilot(_autoPilotPlugin);
	setup->setSource(QUrl::fromUserInput("qrc:/qml/SetupParameterEditor.qml"));
	
	_changeSetupWidget(setup);
}

void SetupView::summaryButtonClicked(void)
{
    Q_ASSERT(_autoPilotPlugin);
    
    QGCQmlWidgetHolder* summary = new QGCQmlWidgetHolder;
    Q_CHECK_PTR(summary);

    summary->setAutoPilot(_autoPilotPlugin);
    summary->setSource(QUrl::fromUserInput("qrc:/qml/VehicleSummary.qml"));

    _changeSetupWidget(summary);
}

void SetupView::setupButtonClicked(const QVariant& component)
{
    if (_uasCurrent->isArmed()) {
        QGCMessageBox::warning("Setup", "Setup cannot be performed while vehicle is armed.");
        return;
    }

    VehicleComponent* vehicle = qobject_cast<VehicleComponent*>(component.value<QObject*>());
    Q_ASSERT(vehicle);
    
    QString setupPrereq = vehicle->prerequisiteSetup();
    if (!setupPrereq.isEmpty()) {
        QGCMessageBox::warning("Setup", QString("%1 setup must be completed prior to %2 setup.").arg(setupPrereq).arg(vehicle->name()));
        return;
    }
    
    _changeSetupWidget(vehicle->setupWidget());
}

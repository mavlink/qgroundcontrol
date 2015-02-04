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
#include "PX4FirmwareUpgrade.h"
#include "ParameterEditor.h"
#include "QGCQmlWidgetHolder.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"

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
    
    //setResizeMode(SizeRootObjectToView);
    
    _ui->buttonHolder->setAutoPilot(NULL);
    _ui->buttonHolder->setSource(QUrl::fromUserInput("qrc:/qml/SetupViewButtons.qml"));
    
    QObject* rootObject = (QObject*)_ui->buttonHolder->rootObject();
    Q_ASSERT(rootObject);
    
    fSucceeded = connect(rootObject, SIGNAL(setupButtonClicked(QVariant)), this, SLOT(_setupButtonClicked(QVariant)));
    Q_ASSERT(fSucceeded);
    
    fSucceeded = connect(rootObject, SIGNAL(firmwareButtonClicked()), this, SLOT(_firmwareButtonClicked()));
    Q_ASSERT(fSucceeded);
    
    fSucceeded = connect(rootObject, SIGNAL(parametersButtonClicked()), this, SLOT(_parametersButtonClicked()));
    Q_ASSERT(fSucceeded);
    
    fSucceeded = connect(rootObject, SIGNAL(summaryButtonClicked()), this, SLOT(_summaryButtonClicked()));
    Q_ASSERT(fSucceeded);
    
    _setActiveUAS(UASManager::instance()->getActiveUAS());
}

SetupView::~SetupView()
{

}

void SetupView::_setActiveUAS(UASInterface* uas)
{
    if (_uasCurrent) {
        Q_ASSERT(_autoPilotPlugin);
        disconnect(_autoPilotPlugin, &AutoPilotPlugin::pluginReady, this, &SetupView::_pluginReady);
    }

    _autoPilotPlugin = NULL;
    _ui->buttonHolder->setAutoPilot(NULL);
    _firmwareButtonClicked();
    QObject* button = _ui->buttonHolder->rootObject()->findChild<QObject*>("firmwareButton");
    Q_ASSERT(button);
    button->setProperty("checked", true);
    
    _uasCurrent = uas;
    
    if (_uasCurrent) {
        _autoPilotPlugin = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uasCurrent);
        
        connect(_autoPilotPlugin, &AutoPilotPlugin::pluginReady, this, &SetupView::_pluginReady);
        if (_autoPilotPlugin->pluginIsReady()) {
            _pluginReady();
        }
    }
}

void SetupView::_pluginReady(void)
{
    _ui->buttonHolder->setAutoPilot(_autoPilotPlugin);
    _summaryButtonClicked();
    QObject* button = _ui->buttonHolder->rootObject()->findChild<QObject*>("summaryButton");
    Q_ASSERT(button);
    button->setProperty("checked", true);
}

void SetupView::_changeSetupWidget(QWidget* newWidget)
{
    if (_currentSetupWidget) {
        delete _currentSetupWidget;
    }
    _currentSetupWidget = newWidget;
    _ui->setupWidgetLayout->addWidget(newWidget);
}

void SetupView::_firmwareButtonClicked(void)
{
    if (_uasCurrent && _uasCurrent->isArmed()) {
        QGCMessageBox::warning("Setup", "Firmware Update cannot be performed while vehicle is armed.");
        return;
    }
    
    PX4FirmwareUpgrade* setup = new PX4FirmwareUpgrade(this);
    _changeSetupWidget(setup);
}

void SetupView::_parametersButtonClicked(void)
{
    ParameterEditor* setup = new ParameterEditor(_uasCurrent, QStringList(), this);
    _changeSetupWidget(setup);
}

void SetupView::_summaryButtonClicked(void)
{
    Q_ASSERT(_autoPilotPlugin);
    
    QGCQmlWidgetHolder* summary = new QGCQmlWidgetHolder;
    Q_CHECK_PTR(summary);

    summary->setAutoPilot(_autoPilotPlugin);
    summary->setSource(QUrl::fromUserInput("qrc:/qml/VehicleSummary.qml"));

    _changeSetupWidget(summary);
}

void SetupView::_setupButtonClicked(const QVariant& component)
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

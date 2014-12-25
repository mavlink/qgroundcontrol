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

#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "VehicleComponent.h"
#include "PX4FirmwareUpgrade.h"
#include "ParameterEditor.h"
#include "SetupWidgetHolder.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"

#include <QQmlError>
#include <QQmlContext>
#include <QDebug>

SetupView::SetupView(QWidget* parent) :
    QGCQuickWidget(parent),
    _uasCurrent(NULL),
    _initComplete(false),
    _autoPilotPlugin(NULL)
{
    bool fSucceeded = connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    Q_UNUSED(fSucceeded);
    Q_ASSERT(fSucceeded);
    
    _setActiveUAS(NULL);
}

SetupView::~SetupView()
{

}

void SetupView::_setActiveUAS(UASInterface* uas)
{
    if (_uasCurrent) {
        Q_ASSERT(_autoPilotPlugin);
        disconnect(_autoPilotPlugin);
        _autoPilotPlugin = NULL;
    }

    _uasCurrent = uas;
    
    if (_uasCurrent) {
        _autoPilotPlugin = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uasCurrent);
        
        connect(_autoPilotPlugin, &AutoPilotPlugin::pluginReady, this, &SetupView::_pluginReady);
        if (_autoPilotPlugin->pluginIsReady()) {
            _setConnectedView();
        }
    } else {
        _setDisconnectedView();
    }
}

void SetupView::_pluginReady(void)
{
    _setConnectedView();
}

void SetupView::_setViewConnections(void)
{
    QObject*button = rootObject()->findChild<QObject*>("firmwareButton");
    Q_ASSERT(button);
    connect(button, SIGNAL(clicked()), this, SLOT(_firmwareButtonClicked()));
    
    button = rootObject()->findChild<QObject*>("parametersButton");
    if (button) {
        connect(button, SIGNAL(clicked()), this, SLOT(_parametersButtonClicked()));
    }
}

void SetupView::_setDisconnectedView(void)
{
    setSource(QUrl::fromUserInput("qrc:qml/SetupViewDisconnected.qml"));
    _setViewConnections();
}

void SetupView::_setConnectedView(void)
{
    Q_ASSERT(_uasCurrent);
    Q_ASSERT(_autoPilotPlugin);
    
    setAutoPilot(_autoPilotPlugin);
    
    setSource(QUrl::fromUserInput("qrc:qml/SetupViewConnected.qml"));
    disconnect(_autoPilotPlugin);
    _setViewConnections();
    
    connect((QObject*)rootObject(), SIGNAL(buttonClicked(QVariant)), this, SLOT(_setupButtonClicked(QVariant)));
}

void SetupView::_firmwareButtonClicked(void)
{
    if (_uasCurrent->isArmed()) {
        QGCMessageBox::warning("Setup", "Firmware Update cannot be performed while vehicle is armed.");
        return;
    }
    
    SetupWidgetHolder* dialog = new SetupWidgetHolder(MainWindow::instance());
    dialog->setModal(true);
    dialog->setWindowTitle("Firmware Upgrade");
    
    PX4FirmwareUpgrade* setup = new PX4FirmwareUpgrade(dialog);
    dialog->setInnerWidget(setup);
    dialog->exec();
}

void SetupView::_parametersButtonClicked(void)
{
    SetupWidgetHolder* dialog = new SetupWidgetHolder(MainWindow::instance());
    dialog->setModal(true);
    dialog->setWindowTitle("Parameter Editor");
    
    ParameterEditor* setup = new ParameterEditor(_uasCurrent, QStringList(), dialog);
    dialog->setInnerWidget(setup);
    dialog->exec();
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
    
    SetupWidgetHolder dialog(MainWindow::instance());
    dialog.setModal(true);
    dialog.setWindowTitle(vehicle->name());
    
    QWidget* setupWidget = vehicle->setupWidget();
    
    dialog.resize(setupWidget->minimumSize());
    dialog.setInnerWidget(setupWidget);
    dialog.exec();
    
    delete setupWidget;
}

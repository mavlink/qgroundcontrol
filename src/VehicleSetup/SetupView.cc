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
#include "VehicleComponentButton.h"
#include "SummaryPage.h"
#include "PX4FirmwareUpgrade.h"

#
#include <QQmlError>
#include <QQmlContext>
#include <QDebug>

SetupView::SetupView(QWidget* parent) :
    QWidget(parent),
    _uasCurrent(NULL),
    _setupWidget(NULL),
    _parameterWidget(NULL),
    _initComplete(false),
    _ui(new Ui::SetupView)
{
    _ui->setupUi(this);
    
    bool fSucceeded = connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    Q_UNUSED(fSucceeded);
    Q_ASSERT(fSucceeded);
    
    connect(_ui->firmwareButton, &QPushButton::clicked, this, &SetupView::_firmwareButtonClicked);
    connect(_ui->summaryButton, &QPushButton::clicked, this, &SetupView::_summaryButtonClicked);
    
    // Summary button is not shown until we have parameters ready
    _ui->summaryButton->setVisible(false);
    
    // We show firmware upgrade until we get parameters
    _firmwareButtonClicked();
}

SetupView::~SetupView()
{
    delete _ui;
}

void SetupView::_setActiveUAS(UASInterface* uas)
{
    if (_uasCurrent) {
        disconnect(_uasCurrent->getParamManager(), SIGNAL(parameterListUpToDate()), this, SLOT(_parametersReady()));
    }
    
    // Clear all UI and fall back to Firmware ui since that is the only thing available when no UAS
    _clearWidgets();
    _clearComponentButtons();
    _ui->summaryButton->setVisible(false);
    _components.clear();
    _firmwareButtonClicked();
    
    _uasCurrent = uas;
    
    if (_uasCurrent) {
        connect(_uasCurrent, &UASInterface::connected, this, &SetupView::_uasConnected);
        connect(_uasCurrent, &UASInterface::disconnected, this, &SetupView::_uasDisconnected);
        
        bool fSucceeded = connect(_uasCurrent->getParamManager(), SIGNAL(parameterListUpToDate()), this, SLOT(_parametersReady()));
        Q_ASSERT(fSucceeded);
        Q_UNUSED(fSucceeded);
        if (_uasCurrent->getParamManager()->parametersReady()) {
            _parametersReady();
        }
    }
}

/// @brief Removes and deletes both the setup and parameter widgets from the tab view.
void SetupView::_clearWidgets(void)
{
    if (_setupWidget) {
        _ui->setupLayout->removeWidget(_setupWidget);
        delete _setupWidget;
        _setupWidget = NULL;
    }
    
    if (_parameterWidget) {
        _ui->setupLayout->removeWidget(_parameterWidget);
        delete _parameterWidget;
        _parameterWidget = NULL;
    }
}

/// @brief Removes and deletes all vehicle component setup buttons from the view.
void SetupView::_clearComponentButtons(void)
{
    QLayoutItem* item;
    while ((item = _ui->componentButtonLayout->itemAt(0))) {
        VehicleComponentButton* componentButton = dynamic_cast<VehicleComponentButton*>(item->widget());
        // Make sure this is really a VehicleComponentButton. If it isn't the UI has changed but the code hasn't.
        Q_ASSERT(componentButton);
        Q_UNUSED(componentButton);
        _ui->componentButtonLayout->removeWidget(item->widget());
    }
}

void SetupView::_summaryButtonClicked(void)
{
    // Clear previous tab widgets
    _clearWidgets();
    
    // Create new tab widgets

    _setupWidget = new SummaryPage(_components);
    Q_CHECK_PTR(_setupWidget);
    _ui->setupLayout->addWidget(_setupWidget);
    
    _parameterWidget = new ParameterEditor(_uasCurrent, QStringList(), this);
    _ui->parameterLayout->addWidget(_parameterWidget);

    _showBothTabs();
    _ui->tabWidget->setTabText(0, tr("Vehicle Summary"));
    _ui->tabWidget->setTabText(1, tr("All Parameters"));
    
    _uncheckAllButtons();
    _ui->summaryButton->setChecked(true);
}

void SetupView::_firmwareButtonClicked(void)
{
    // Clear previous tab widgets
    _clearWidgets();
    
    // Create new tab widgets
    
    _setupWidget = new PX4FirmwareUpgrade(this);
    Q_CHECK_PTR(_setupWidget);
    _ui->setupLayout->addWidget(_setupWidget);
    
    _showOnlySetupTab();
    _ui->tabWidget->setTabEnabled(1, false);
    _ui->tabWidget->setTabText(0, tr("Firmware Upgrade"));

    _uncheckAllButtons();
    _ui->firmwareButton->setChecked(true);
}

void SetupView::_componentButtonClicked(void)
{
    // Clear previous tab widgets
    _clearWidgets();
    
    // Create new tab widgets
    
    VehicleComponentButton* componentButton = dynamic_cast<VehicleComponentButton*>(sender());
    Q_ASSERT(componentButton);
    
    VehicleComponent* component = componentButton->component();
    Q_ASSERT(component);
    
    _setupWidget = component->setupWidget();
    Q_ASSERT(_setupWidget);
    _ui->setupLayout->addWidget(_setupWidget);
    
    _parameterWidget = new ParameterEditor(_uasCurrent, component->paramFilterList(), this);
    _ui->parameterLayout->addWidget(_parameterWidget);
    
    _showBothTabs();
    _ui->tabWidget->setTabText(0, tr("%1 Setup").arg(component->name()));
    _ui->tabWidget->setTabText(1, tr("%1 Parameters").arg(component->name()));

    _uncheckAllButtons();
    componentButton->setChecked(true);
}

void SetupView::_parametersReady(void)
{
    // When parameters become ready for the first time, setup the rest of the ui.
    
    if (_initComplete) {
        return;
    }
    
    _initComplete = true;
    
    disconnect(_uasCurrent->getParamManager(), SIGNAL(parameterListUpToDate()), this, SLOT(_parametersReady()));
    
    _ui->summaryButton->setVisible(true);
    
    _components = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uasCurrent->getAutopilotType())->getVehicleComponents(_uasCurrent);
    
    foreach(VehicleComponent* component, _components) {
        VehicleComponentButton* button = new VehicleComponentButton(component, _ui->navBarWidget);
        
        button->setCheckable(true);
        
        button->setObjectName(component->name() + "Button");
        button->setText(component->name());
        
        QIcon icon;
        icon.addFile(component->icon());
        button->setIcon(icon);
        button->setIconSize(_ui->firmwareButton->iconSize());
        
        connect(button, &VehicleComponentButton::clicked, this, &SetupView::_componentButtonClicked);
        
        _ui->componentButtonLayout->addWidget(button);
    }
    
    _summaryButtonClicked();
}

void SetupView::_showOnlySetupTab(void)
{
    _ui->tabWidget->setCurrentIndex(0);
    _ui->tabWidget->removeTab(1);
}

void SetupView::_showBothTabs(void)
{
    _ui->tabWidget->setCurrentIndex(0);
    if (_ui->tabWidget->count() == 1) {
        _ui->tabWidget->insertTab(1, _ui->parameterTab, QString());
    }
}

void SetupView::_uncheckAllButtons(void)
{
    _ui->firmwareButton->setChecked(false);
    _ui->summaryButton->setChecked(false);
    
    int i = 0;
    QLayoutItem* item;
    while ((item = _ui->componentButtonLayout->itemAt(i++))) {
        VehicleComponentButton* componentButton = dynamic_cast<VehicleComponentButton*>(item->widget());
        if (componentButton != NULL) {
            componentButton->setChecked(false);
        }
    }
}

void SetupView::_uasConnected(void)
{
    qDebug() << "UAS connected";
    // FIXME: We should re-connect the UI
}

void SetupView::_uasDisconnected(void)
{
    qDebug() << "UAS disconnected";
    // FIXME: We should disconnect the UI
}

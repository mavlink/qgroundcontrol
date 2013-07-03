#include "ApmSoftwareConfig.h"


ApmSoftwareConfig::ApmSoftwareConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    ui.basicPidsButton->setVisible(false);
    ui.flightModesButton->setVisible(false);
    ui.standardParamButton->setVisible(false);
    ui.geoFenceButton->setVisible(false);
    ui.failSafeButton->setVisible(false);
    ui.advancedParamButton->setVisible(false);
    ui.advParamListButton->setVisible(false);
    ui.arduCoperPidButton->setVisible(false);

    basicPidConfig = new BasicPidConfig(this);
    ui.stackedWidget->addWidget(basicPidConfig);
    buttonToConfigWidgetMap[ui.basicPidsButton] = basicPidConfig;
    connect(ui.basicPidsButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    flightConfig = new FlightModeConfig(this);
    ui.stackedWidget->addWidget(flightConfig);
    buttonToConfigWidgetMap[ui.flightModesButton] = flightConfig;
    connect(ui.flightModesButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    standardParamConfig = new StandardParamConfig(this);
    ui.stackedWidget->addWidget(standardParamConfig);
    buttonToConfigWidgetMap[ui.standardParamButton] = standardParamConfig;
    connect(ui.standardParamButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    geoFenceConfig = new GeoFenceConfig(this);
    ui.stackedWidget->addWidget(geoFenceConfig);
    buttonToConfigWidgetMap[ui.geoFenceButton] = geoFenceConfig;
    connect(ui.geoFenceButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    failSafeConfig = new FailSafeConfig(this);
    ui.stackedWidget->addWidget(failSafeConfig);
    buttonToConfigWidgetMap[ui.failSafeButton] = failSafeConfig;
    connect(ui.failSafeButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    advancedParamConfig = new AdvancedParamConfig(this);
    ui.stackedWidget->addWidget(advancedParamConfig);
    buttonToConfigWidgetMap[ui.advancedParamButton] = advancedParamConfig;
    connect(ui.advancedParamButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    arduCoperPidConfig = new ArduCopterPidConfig(this);
    ui.stackedWidget->addWidget(arduCoperPidConfig);
    buttonToConfigWidgetMap[ui.arduCoperPidButton] = arduCoperPidConfig;
    connect(ui.arduCoperPidButton,SIGNAL(clicked()),this,SLOT(activateStackedWidget()));

    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        activeUASSet(UASManager::instance()->getActiveUAS());
    }
}

ApmSoftwareConfig::~ApmSoftwareConfig()
{
}
void ApmSoftwareConfig::activateStackedWidget()
{
    if (buttonToConfigWidgetMap.contains(sender()))
    {
        ui.stackedWidget->setCurrentWidget(buttonToConfigWidgetMap[sender()]);
    }
}
void ApmSoftwareConfig::activeUASSet(UASInterface *uas)
{
    if (!uas)
    {
        return;
    }

    ui.basicPidsButton->setVisible(true);
    ui.flightModesButton->setVisible(true);
    ui.standardParamButton->setVisible(true);
    ui.geoFenceButton->setVisible(true);
    ui.failSafeButton->setVisible(true);
    ui.advancedParamButton->setVisible(true);
    ui.advParamListButton->setVisible(true);

    ui.arduCoperPidButton->setVisible(true);
}

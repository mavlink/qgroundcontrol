// On Windows (for VS2010) stdint.h contains the limits normally contained in limits.h
// It also needs the __STDC_LIMIT_MACROS macro defined in order to include them (done
// in qgroundcontrol.pri).
#ifdef WIN32
#include <stdint.h>
#else
#include <limits.h>
#endif

#include <QTimer>
#include <QDir>
#include <QXmlStreamReader>
#include <QLabel>

#include "QGCPX4VehicleConfig.h"

#include "QGC.h"
#include "QGCToolWidget.h"
#include "UASManager.h"
#include "LinkManager.h"
#include "UASParameterCommsMgr.h"
#include "ui_QGCPX4VehicleConfig.h"
#include "px4_configuration/QGCPX4AirframeConfig.h"
#include "px4_configuration/QGCPX4SensorCalibration.h"
#include "px4_configuration/PX4RCCalibration.h"

#include "PX4FirmwareUpgrade.h"

#define WIDGET_INDEX_FIRMWARE 0
#define WIDGET_INDEX_RC 1
#define WIDGET_INDEX_SENSOR_CAL 2
#define WIDGET_INDEX_AIRFRAME_CONFIG 3
#define WIDGET_INDEX_GENERAL_CONFIG 4
#define WIDGET_INDEX_ADV_CONFIG 5

#define MIN_PWM_VAL 800
#define MAX_PWM_VAL 2200

QGCPX4VehicleConfig::QGCPX4VehicleConfig(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    px4AirframeConfig(NULL),
    planeBack(":/files/images/px4/rc/cessna_back.png"),
    planeSide(":/files/images/px4/rc/cessna_side.png"),
    px4SensorCalibration(NULL),
    ui(new Ui::QGCPX4VehicleConfig)
{
    doneLoadingConfig = false;

    setObjectName("QGC_VEHICLECONFIG");
    ui->setupUi(this);

    ui->advancedMenuButton->setEnabled(false);
    ui->airframeMenuButton->setEnabled(false);
    ui->sensorMenuButton->setEnabled(false);
    ui->rcMenuButton->setEnabled(false);

    px4AirframeConfig = new QGCPX4AirframeConfig(this);
    ui->airframeLayout->addWidget(px4AirframeConfig);

    px4SensorCalibration = new QGCPX4SensorCalibration(this);
    ui->sensorLayout->addWidget(px4SensorCalibration);

    px4RCCalibration = new PX4RCCalibration(this);
    ui->rcLayout->addWidget(px4RCCalibration);
    
    PX4FirmwareUpgrade* firmwareUpgrade = new PX4FirmwareUpgrade(this);
    ui->firmwareLayout->addWidget(firmwareUpgrade);

    connect(ui->rcMenuButton,SIGNAL(clicked()),
            this,SLOT(rcMenuButtonClicked()));
    connect(ui->sensorMenuButton,SIGNAL(clicked()),
            this,SLOT(sensorMenuButtonClicked()));
    connect(ui->flightModeMenuButton, SIGNAL(clicked()),
            this, SLOT(flightModeMenuButtonClicked()));
    connect(ui->safetyConfigButton, SIGNAL(clicked()),
            this, SLOT(safetyConfigMenuButtonClicked()));
    connect(ui->tuningMenuButton,SIGNAL(clicked()),
            this,SLOT(tuningMenuButtonClicked()));
    connect(ui->advancedMenuButton,SIGNAL(clicked()),
            this,SLOT(advancedMenuButtonClicked()));
    connect(ui->airframeMenuButton, SIGNAL(clicked()),
            this, SLOT(airframeMenuButtonClicked()));
    connect(ui->firmwareMenuButton, SIGNAL(clicked()),
            this, SLOT(firmwareMenuButtonClicked()));

    //TODO connect buttons here to save/clear actions?
    UASInterface* tmpMav = UASManager::instance()->getActiveUAS();
    if (tmpMav) {
        ui->pendingCommitsWidget->initWithUAS(tmpMav);
        ui->pendingCommitsWidget->update();
        setActiveUAS(tmpMav);
    }

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));

    firmwareMenuButtonClicked();
}

QGCPX4VehicleConfig::~QGCPX4VehicleConfig()
{
    delete ui;
}

void QGCPX4VehicleConfig::rcMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->rcTab);
    ui->tabTitleLabel->setText(tr("Radio Calibration"));
}

void QGCPX4VehicleConfig::sensorMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->sensorTab);
    ui->tabTitleLabel->setText(tr("Sensor Calibration"));
}

void QGCPX4VehicleConfig::tuningMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->tuningTab);
    ui->tabTitleLabel->setText(tr("Controller Tuning"));
}

void QGCPX4VehicleConfig::flightModeMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->flightModeTab);
    ui->tabTitleLabel->setText(tr("Flight Mode Configuration"));
}

void QGCPX4VehicleConfig::safetyConfigMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->safetyConfigTab);
    ui->tabTitleLabel->setText(tr("Safety Feature Configuration"));
}

void QGCPX4VehicleConfig::advancedMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->advancedTab);
    ui->tabTitleLabel->setText(tr("Advanced Configuration Options"));
}

void QGCPX4VehicleConfig::airframeMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->airframeTab);
    ui->tabTitleLabel->setText(tr("Airframe Configuration"));
}

void QGCPX4VehicleConfig::firmwareMenuButtonClicked()
{
    ui->stackedWidget->setCurrentWidget(ui->firmwareTab);
    ui->tabTitleLabel->setText(tr("Firmware Upgrade"));
}

void QGCPX4VehicleConfig::menuButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button)
    {
        return;
    }
    if (buttonToWidgetMap.contains(button))
    {
        ui->stackedWidget->setCurrentWidget(buttonToWidgetMap[button]);
    }

}

void QGCPX4VehicleConfig::setActiveUAS(UASInterface* active)
{
    // Hide items if NULL and abort
    if (!active) {
        return;
    }


    // Do nothing if UAS is already visible
    if (mav == active)
        return;

    if (mav)
    {

        //TODO use paramCommsMgr instead
        disconnect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
                   SLOT(parameterChanged(int,int,QString,QVariant)));

        foreach(QWidget* child, ui->airframeLayout->findChildren<QWidget*>())
        {
            child->deleteLater();
        }

        // And then delete any custom tabs
        foreach(QWidget* child, additionalTabs) {
            child->deleteLater();
        }
        additionalTabs.clear();

        toolWidgets.clear();
        paramToWidgetMap.clear();
        libParamToWidgetMap.clear();
        systemTypeToParamMap.clear();
        toolToBoxMap.clear();
        paramTooltips.clear();
    }

    // Connect new system
    mav = active;

    paramMgr = mav->getParamManager();

    ui->pendingCommitsWidget->setUAS(mav);
    ui->paramTreeWidget->setUAS(mav);

    //TODO eliminate the separate RC_TYPE call
    mav->requestParameter(0, "RC_TYPE");

    // Connect new system
    connect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
               SLOT(parameterChanged(int,int,QString,QVariant)));


    if (systemTypeToParamMap.contains(mav->getSystemTypeName())) {
        paramToWidgetMap = systemTypeToParamMap[mav->getSystemTypeName()];
    }
    else {
        //Indication that we have no meta data for this system type.
        qDebug() << "No parameters defined for system type:" << mav->getSystemTypeName();
        paramToWidgetMap = systemTypeToParamMap[mav->getSystemTypeName()];
    }

    if (!paramTooltips.isEmpty()) {
           mav->getParamManager()->setParamDescriptions(paramTooltips);
    }

    qDebug() << "CALIBRATION!! System Type Name:" << mav->getSystemTypeName();

    updateStatus(QString("Reading from system %1").arg(mav->getUASName()));

    // Since a system is now connected, enable the VehicleConfig UI.
    // Enable buttons
    
    bool px4Firmware = mav->getAutopilotType() == MAV_AUTOPILOT_PX4;
    ui->airframeMenuButton->setEnabled(px4Firmware);
    ui->sensorMenuButton->setEnabled(px4Firmware);
    ui->rcMenuButton->setEnabled(px4Firmware);
    ui->advancedMenuButton->setEnabled(true);
}

void QGCPX4VehicleConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (!doneLoadingConfig) {
        //We do not want to attempt to generate any UI elements until loading of the config file is complete.
        //We should re-request params later if needed, that is not implemented yet.
        return;
    }

    if (paramToWidgetMap.contains(parameterName)) {
        //Main group of parameters of the selected airframe
        paramToWidgetMap.value(parameterName)->setParameterValue(uas,component,parameterName,value);
        if (toolToBoxMap.contains(paramToWidgetMap.value(parameterName))) {
            toolToBoxMap[paramToWidgetMap.value(parameterName)]->show();
        }
        else {
            qCritical() << "Widget with no box, possible memory corruption for param:" << parameterName;
        }
    }
    else if (libParamToWidgetMap.contains(parameterName)) {
        //All the library parameters
        libParamToWidgetMap.value(parameterName)->setParameterValue(uas,component,parameterName,value);
        if (toolToBoxMap.contains(libParamToWidgetMap.value(parameterName))) {
            toolToBoxMap[libParamToWidgetMap.value(parameterName)]->show();
        }
        else {
            qCritical() << "Widget with no box, possible memory corruption for param:" << parameterName;
        }
    }
    else {
        //Param recieved that we have no metadata for. Search to see if it belongs in a
        //group with some other params
        //bool found = false;
        for (int i=0;i<toolWidgets.size();i++) {
            if (parameterName.startsWith(toolWidgets[i]->objectName())) {
                //It should be grouped with this one, add it.
                toolWidgets[i]->addParam(uas,component,parameterName,value);
                libParamToWidgetMap.insert(parameterName,toolWidgets[i]);
                //found  = true;
                break;
            }
        }
//        if (!found) {
//            //New param type, create a QGroupBox for it.
//            QWidget* parent = ui->advanceColumnContents;

//            // Create the tool, attaching it to the QGroupBox
//            QGCToolWidget *tool = new QGCToolWidget("", parent);
//            QString tooltitle = parameterName;
//            if (parameterName.split("_").size() > 1) {
//                tooltitle = parameterName.split("_")[0] + "_";
//            }
//            tool->setTitle(tooltitle);
//            tool->setObjectName(tooltitle);
//            //tool->setSettings(set);
//            libParamToWidgetMap.insert(parameterName,tool);
//            toolWidgets.append(tool);
//            tool->addParam(uas, component, parameterName, value);
//            QGroupBox *box = new QGroupBox(parent);
//            box->setTitle(tool->objectName());
//            box->setLayout(new QVBoxLayout(box));
//            box->layout()->addWidget(tool);

//            libParamToWidgetMap.insert(parameterName,tool);
//            toolWidgets.append(tool);
//            ui->advancedColumnLayout->addWidget(box);

//            toolToBoxMap[tool] = box;
//        }
    }

}

void QGCPX4VehicleConfig::updateStatus(const QString& str)
{
    ui->advancedStatusLabel->setText(str);
    ui->advancedStatusLabel->setStyleSheet("");
}

void QGCPX4VehicleConfig::updateError(const QString& str)
{
    ui->advancedStatusLabel->setText(str);
    ui->advancedStatusLabel->setStyleSheet(QString("QLabel { margin: 0px 2px; font: 14px; color: %1; background-color: %2; }").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.name()));
}

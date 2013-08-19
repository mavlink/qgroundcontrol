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
#include <QMessageBox>
#include <QLabel>

#include "QGCPX4VehicleConfig.h"

#include "QGC.h"
#include "QGCPendingParamWidget.h"
#include "QGCToolWidget.h"
#include "UASManager.h"
#include "UASParameterCommsMgr.h"
#include "ui_QGCPX4VehicleConfig.h"
#include "px4_configuration/QGCPX4AirframeConfig.h"
#include <dialog_bare.h>

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
    chanCount(0),
    rcRoll(0.0f),
    rcPitch(0.0f),
    rcYaw(0.0f),
    rcThrottle(0.0f),
    rcMode(0.0f),
    rcAux1(0.0f),
    rcAux2(0.0f),
    rcAux3(0.0f),
    dataModelChanged(true),
    channelWanted(-1),
    rc_mode(RC_MODE_NONE),
    calibrationEnabled(false),
    px4AirframeConfig(NULL),
    #ifdef QUPGRADE_SUPPORT
    firmwareDialog(NULL),
    #endif
    ui(new Ui::QGCPX4VehicleConfig)
{
    doneLoadingConfig = false;

    channelNames << "Roll / Aileron";
    channelNames << "Pitch / Elevator";
    channelNames << "Yaw / Rudder";
    channelNames << "Throttle";
    channelNames << "SW1 / Main Mode Switch";
    channelNames << "SW2 / Sub Mode Switch";
    channelNames << "Aux1 / Flaps";
    channelNames << "Aux2";
    channelNames << "Aux3";
    channelNames << "Aux4";
    channelNames << "Aux5";
    channelNames << "Aux6";

    setObjectName("QGC_VEHICLECONFIG");
    ui->setupUi(this);

    px4AirframeConfig = new QGCPX4AirframeConfig(this);
    ui->airframeLayout->addWidget(px4AirframeConfig);

#ifdef QUPGRADE_SUPPORT
    firmwareDialog = new DialogBare(this);
    ui->firmwareLayout->addWidget(firmwareDialog);
#else
#error Please check out QUpgrade from http://github.com/LorenzMeier/qupgrade/ into the QGroundControl folder.

    QLabel* label = new QLabel(this);
    label->setText("THIS VERSION OF QGROUNDCONTROL WAS BUILT WITHOUT QUPGRADE. To enable firmware upload support, checkout QUpgrade WITHIN the QGroundControl folder");
    ui->firmwareLayout->addWidget(label);
#endif

    ui->rollWidget->setOrientation(Qt::Horizontal);
    ui->rollWidget->setName("Roll");
    ui->yawWidget->setOrientation(Qt::Horizontal);
    ui->yawWidget->setName("Yaw");
    ui->pitchWidget->setName("Pitch");
    ui->throttleWidget->setName("Throttle");
    ui->radio5Widget->setOrientation(Qt::Horizontal);
    ui->radio5Widget->setName("Radio 5");
    ui->radio6Widget->setOrientation(Qt::Horizontal);
    ui->radio6Widget->setName("Radio 6");
    ui->radio7Widget->setOrientation(Qt::Horizontal);
    ui->radio7Widget->setName("Radio 7");
    ui->radio8Widget->setOrientation(Qt::Horizontal);
    ui->radio8Widget->setName("Radio 8");

    connect(ui->rcMenuButton,SIGNAL(clicked()),
            this,SLOT(rcMenuButtonClicked()));
    connect(ui->sensorMenuButton,SIGNAL(clicked()),
            this,SLOT(sensorMenuButtonClicked()));
    connect(ui->generalMenuButton,SIGNAL(clicked()),
            this,SLOT(generalMenuButtonClicked()));
    connect(ui->advancedMenuButton,SIGNAL(clicked()),
            this,SLOT(advancedMenuButtonClicked()));
    connect(ui->airframeMenuButton, SIGNAL(clicked()),
            this, SLOT(airframeMenuButtonClicked()));
    connect(ui->firmwareMenuButton, SIGNAL(clicked()),
            this, SLOT(firmwareMenuButtonClicked()));

    ui->rcCalibrationButton->setCheckable(true);
    connect(ui->rcCalibrationButton, SIGNAL(clicked(bool)), this, SLOT(toggleCalibrationRC(bool)));
    connect(ui->writeButton, SIGNAL(clicked()),
            this, SLOT(writeParameters()));

    //TODO connect buttons here to save/clear actions?
    UASInterface* tmpMav = UASManager::instance()->getActiveUAS();
    if (tmpMav) {
        ui->pendingCommitsWidget->initWithUAS(tmpMav);
        ui->pendingCommitsWidget->update();
        setActiveUAS(tmpMav);
    }

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));

    // Connect RC mapping assignments
    connect(ui->rollSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setRollChan(int)));
    connect(ui->pitchSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPitchChan(int)));
    connect(ui->yawSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setYawChan(int)));
    connect(ui->throttleSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setThrottleChan(int)));
    connect(ui->modeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setModeChan(int)));
    connect(ui->aux1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAux1Chan(int)));
    connect(ui->aux2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAux2Chan(int)));
    connect(ui->aux3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAux3Chan(int)));

    // Connect RC reverse assignments
    connect(ui->invertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setRollInverted(bool)));
    connect(ui->invertCheckBox_2, SIGNAL(clicked(bool)), this, SLOT(setPitchInverted(bool)));
    connect(ui->invertCheckBox_3, SIGNAL(clicked(bool)), this, SLOT(setYawInverted(bool)));
    connect(ui->invertCheckBox_4, SIGNAL(clicked(bool)), this, SLOT(setThrottleInverted(bool)));
    connect(ui->invertCheckBox_5, SIGNAL(clicked(bool)), this, SLOT(setModeInverted(bool)));
    connect(ui->invertCheckBox_6, SIGNAL(clicked(bool)), this, SLOT(setAux1Inverted(bool)));
    connect(ui->invertCheckBox_7, SIGNAL(clicked(bool)), this, SLOT(setAux2Inverted(bool)));
    connect(ui->invertCheckBox_8, SIGNAL(clicked(bool)), this, SLOT(setAux3Inverted(bool)));

    connect(ui->rollButton, SIGNAL(clicked()), this, SLOT(identifyRollChannel()));
    connect(ui->pitchButton, SIGNAL(clicked()), this, SLOT(identifyPitchChannel()));
    connect(ui->yawButton, SIGNAL(clicked()), this, SLOT(identifyYawChannel()));
    connect(ui->throttleButton, SIGNAL(clicked()), this, SLOT(identifyThrottleChannel()));
    connect(ui->modeButton, SIGNAL(clicked()), this, SLOT(identifyModeChannel()));
    connect(ui->subButton, SIGNAL(clicked()), this, SLOT(identifySubModeChannel()));
    connect(ui->aux1Button, SIGNAL(clicked()), this, SLOT(identifyAux1Channel()));
    connect(ui->aux2Button, SIGNAL(clicked()), this, SLOT(identifyAux2Channel()));

    for (unsigned int i = 0; i < chanMax; i++) {
        rcValue[i] = UINT16_MAX;
        rcMapping[i] = i;
        channelWantedList[i] = UINT16_MAX;
        rcMin[i] = 1000;
        rcMax[i] = 2000;
    }

    updateTimer.setInterval(150);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateView()));
    updateTimer.start();
}

void QGCPX4VehicleConfig::rcMenuButtonClicked()
{
    //TODO eg ui->stackedWidget->findChild("rcConfig");
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_RC);
}

void QGCPX4VehicleConfig::sensorMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_SENSOR_CAL);
}

void QGCPX4VehicleConfig::generalMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_GENERAL_CONFIG);
}

void QGCPX4VehicleConfig::advancedMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_ADV_CONFIG);
}

void QGCPX4VehicleConfig::airframeMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_AIRFRAME_CONFIG);
}

void QGCPX4VehicleConfig::firmwareMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_FIRMWARE);
}

void QGCPX4VehicleConfig::identifyChannelMapping(int aert_index)
{
    if (chanCount == 0)
        return;
    channelWanted = aert_index;

    for (unsigned i = 0; i < sizeof(channelWantedList) / sizeof(channelWantedList[0]); i++)
    {
        if (i >= chanCount) {
            channelWantedList[i] = 0;
        } else {
            channelWantedList[i] = rcValue[i];
        }
    }

}

QGCPX4VehicleConfig::~QGCPX4VehicleConfig()
{
    delete ui;
}

void QGCPX4VehicleConfig::setRCModeIndex(int newRcMode)
{
    newRcMode += 1; //promote from an index to a mode enum
    if (newRcMode > 0 && newRcMode < 5) {
        rc_mode = (enum RC_MODE) (newRcMode);
        dataModelChanged = true;
    }
}

void QGCPX4VehicleConfig::toggleCalibrationRC(bool enabled)
{
    if (enabled)
    {
        startCalibrationRC();
    }
    else
    {
        stopCalibrationRC();
    }
}

void QGCPX4VehicleConfig::setTrimPositions()
{
    // Set trim to min if stick is close to min
    if (abs(rcValue[rcMapping[3]] - rcMin[rcMapping[3]]) < 100)
    {
        rcTrim[rcMapping[3]] = rcMin[rcMapping[3]];   // throttle
    }
    // Set trim to max if stick is close to max
    else if (abs(rcValue[rcMapping[3]] - rcMax[rcMapping[3]]) < 100)
    {
        rcTrim[rcMapping[3]] = rcMax[rcMapping[3]];   // throttle
    }
    else
    {
        // Reject
        QMessageBox msgBox;
        msgBox.setText(tr("Throttle Stick Trim Position Invalid"));
        msgBox.setInformativeText(tr("The throttle stick is not in the min position. Please set it to the minimum value"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        (void)msgBox.exec();
    }

    // Set trim for roll, pitch, yaw, throttle
    rcTrim[rcMapping[0]] = rcValue[rcMapping[0]]; // roll
    rcTrim[rcMapping[1]] = rcValue[rcMapping[1]]; // pitch
    rcTrim[rcMapping[2]] = rcValue[rcMapping[2]]; // yaw

    rcTrim[rcMapping[4]] = ((rcMax[rcMapping[4]] - rcMin[rcMapping[4]]) / 2.0f) + rcMin[rcMapping[4]];   // mode sw
    rcTrim[rcMapping[5]] = ((rcMax[rcMapping[5]] - rcMin[rcMapping[5]]) / 2.0f) + rcMin[rcMapping[5]];   // aux 1
    rcTrim[rcMapping[6]] = ((rcMax[rcMapping[6]] - rcMin[rcMapping[6]]) / 2.0f) + rcMin[rcMapping[6]];   // aux 2
    rcTrim[rcMapping[7]] = ((rcMax[rcMapping[7]] - rcMin[rcMapping[7]]) / 2.0f) + rcMin[rcMapping[7]];   // aux 3
}

void QGCPX4VehicleConfig::detectChannelInversion()
{

}

void QGCPX4VehicleConfig::startCalibrationRC()
{
    QMessageBox::information(0,"Warning!","You are about to start radio calibration.\nPlease ensure all motor power is disconnected AND all props are removed from the vehicle.\nAlso ensure transmitter and reciever are powered and connected\n\nClick OK to confirm");
    QMessageBox::information(0,"Information","Click OK, then move all sticks to their extreme positions, watching the min/max values to ensure you get the most range from your controller. This includes all switches");
    ui->rcCalibrationButton->setText(tr("Stop RC Calibration"));
    resetCalibrationRC();
    calibrationEnabled = true;
    ui->rollWidget->showMinMax();
    ui->pitchWidget->showMinMax();
    ui->yawWidget->showMinMax();
    ui->throttleWidget->showMinMax();
    ui->radio5Widget->showMinMax();
    ui->radio6Widget->showMinMax();
    ui->radio7Widget->showMinMax();
    ui->radio8Widget->showMinMax();
}

void QGCPX4VehicleConfig::stopCalibrationRC()
{
    QMessageBox::information(0,"Trims","Ensure all sticks are centeres and throttle is in the downmost position, click OK to continue");
    calibrationEnabled = false;
    ui->rcCalibrationButton->setText(tr("Start RC Calibration"));

    ui->rollWidget->hideMinMax();
    ui->pitchWidget->hideMinMax();
    ui->yawWidget->hideMinMax();
    ui->throttleWidget->hideMinMax();
    ui->radio5Widget->hideMinMax();
    ui->radio6Widget->hideMinMax();
    ui->radio7Widget->hideMinMax();
    ui->radio8Widget->hideMinMax();

    for (int i=0;i<chanCount;i++)
    {
        if (rcMin[i] > 1350)
            rcMin[i] = 1000;

        if (rcMax[i] < 1650)
            rcMax[i] = 2000;
    }

    QString statusstr;
    statusstr = "Below you will find the detected radio calibration information that will be sent to the autopilot\n";
    statusstr += "Normal values are around 1100 to 1900, with disconnected channels reading 1000, 1500, 2000\n\n";
    statusstr += "Channel\tMin\tCenter\tMax\n";
    statusstr += "--------------------\n";
    for (int i=0;i<chanCount;i++)
    {
        statusstr += QString::number(i) +"\t"+ QString::number(rcMin[i]) +"\t"+ QString::number(rcValue[i]) +"\t"+ QString::number(rcMax[i]) +"\n";
    }
    QMessageBox::information(0,"Status",statusstr);
}

void QGCPX4VehicleConfig::loadQgcConfig(bool primary)
{
    Q_UNUSED(primary);
    QDir autopilotdir(qApp->applicationDirPath() + "/files/" + mav->getAutopilotTypeName().toLower());
    QDir generaldir = QDir(autopilotdir.absolutePath() + "/general/widgets");
    QDir vehicledir = QDir(autopilotdir.absolutePath() + "/" + mav->getSystemTypeName().toLower() + "/widgets");
    if (!autopilotdir.exists("general"))
    {
     //TODO: Throw some kind of error here. There is no general configuration directory
        qWarning() << "Invalid general dir. no general configuration will be loaded.";
    }
    if (!autopilotdir.exists(mav->getAutopilotTypeName().toLower()))
    {
        //TODO: Throw an error here too, no autopilot specific configuration
        qWarning() << "Invalid vehicle dir, no vehicle specific configuration will be loaded.";
    }

    // Generate widgets for the General tab.
    QGCToolWidget *tool;
    bool left = true;
    foreach (QString file,generaldir.entryList(QDir::Files | QDir::NoDotAndDotDot))
    {
        if (file.toLower().endsWith(".qgw")) {
            QWidget* parent = left?ui->generalLeftContents:ui->generalRightContents;
            tool = new QGCToolWidget("", parent);
            if (tool->loadSettings(generaldir.absoluteFilePath(file), false))
            {
                toolWidgets.append(tool);
                QGroupBox *box = new QGroupBox(parent);
                box->setTitle(tool->objectName());
                box->setLayout(new QVBoxLayout(box));
                box->layout()->addWidget(tool);
                if (left)
                {
                    left = false;
                    ui->generalLeftLayout->addWidget(box);
                }
                else
                {
                    left = true;
                    ui->generalRightLayout->addWidget(box);
                }
            } else {
                delete tool;
            }
        }
    }

    // Generate widgets for the Advanced tab.
    foreach (QString file,vehicledir.entryList(QDir::Files | QDir::NoDotAndDotDot))
    {
        if (file.toLower().endsWith(".qgw")) {
            QWidget* parent = ui->advanceColumnContents;
            tool = new QGCToolWidget("", parent);
            if (tool->loadSettings(vehicledir.absoluteFilePath(file), false))
            {
                toolWidgets.append(tool);
                QGroupBox *box = new QGroupBox(parent);
                box->setTitle(tool->objectName());
                box->setLayout(new QVBoxLayout(box));
                box->layout()->addWidget(tool);
                ui->advancedColumnLayout->addWidget(box);

            } else {
                delete tool;
            }
        }
    }

    // Load tabs for general configuration
    foreach (QString dir,generaldir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QPushButton *button = new QPushButton(ui->leftNavScrollAreaWidgetContents);
        connect(button,SIGNAL(clicked()),this,SLOT(menuButtonClicked()));
        ui->navBarLayout->insertWidget(2,button);
        button->setMinimumHeight(75);
        button->setMinimumWidth(100);
        button->show();
        button->setText(dir);
        QWidget *tab = new QWidget(ui->stackedWidget);
        ui->stackedWidget->insertWidget(2,tab);
        buttonToWidgetMap[button] = tab;
        tab->setLayout(new QVBoxLayout());
        tab->show();
        QScrollArea *area = new QScrollArea(tab);
        tab->layout()->addWidget(area);
        QWidget *scrollArea = new QWidget(tab);
        scrollArea->setLayout(new QVBoxLayout(tab));
        area->setWidget(scrollArea);
        area->setWidgetResizable(true);
        area->show();
        scrollArea->show();
        QDir newdir = QDir(generaldir.absoluteFilePath(dir));
        foreach (QString file,newdir.entryList(QDir::Files| QDir::NoDotAndDotDot))
        {
            if (file.toLower().endsWith(".qgw")) {
                tool = new QGCToolWidget("", tab);
                if (tool->loadSettings(newdir.absoluteFilePath(file), false))
                {
                    toolWidgets.append(tool);
                    //ui->sensorLayout->addWidget(tool);
                    QGroupBox *box = new QGroupBox(tab);
                    box->setTitle(tool->objectName());
                    box->setLayout(new QVBoxLayout(tab));
                    box->layout()->addWidget(tool);
                    scrollArea->layout()->addWidget(box);
                } else {
                    delete tool;
                }
            }
        }
    }

    // Load additional tabs for vehicle specific configuration
    foreach (QString dir,vehicledir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QPushButton *button = new QPushButton(ui->leftNavScrollAreaWidgetContents);
        connect(button,SIGNAL(clicked()),this,SLOT(menuButtonClicked()));
        ui->navBarLayout->insertWidget(2,button);

        QWidget *tab = new QWidget(ui->stackedWidget);
        ui->stackedWidget->insertWidget(2,tab);
        buttonToWidgetMap[button] = tab;

        button->setMinimumHeight(75);
        button->setMinimumWidth(100);
        button->show();
        button->setText(dir);
        tab->setLayout(new QVBoxLayout());
        tab->show();
        QScrollArea *area = new QScrollArea(tab);
        tab->layout()->addWidget(area);
        QWidget *scrollArea = new QWidget(tab);
        scrollArea->setLayout(new QVBoxLayout(tab));
        area->setWidget(scrollArea);
        area->setWidgetResizable(true);
        area->show();
        scrollArea->show();

        QDir newdir = QDir(vehicledir.absoluteFilePath(dir));
        foreach (QString file,newdir.entryList(QDir::Files| QDir::NoDotAndDotDot))
        {
            if (file.toLower().endsWith(".qgw")) {
                tool = new QGCToolWidget("", tab);
                tool->addUAS(mav);
                if (tool->loadSettings(newdir.absoluteFilePath(file), false))
                {
                    toolWidgets.append(tool);
                    //ui->sensorLayout->addWidget(tool);
                    QGroupBox *box = new QGroupBox(tab);
                    box->setTitle(tool->objectName());
                    box->setLayout(new QVBoxLayout(box));
                    box->layout()->addWidget(tool);
                    scrollArea->layout()->addWidget(box);
                    box->show();
                    //gbox->layout()->addWidget(box);
                } else {
                    delete tool;
                }
            }
        }
    }


    // Load general calibration for autopilot
    //TODO: Handle this more gracefully, maybe have it scan the directory for multiple calibration entries?
    tool = new QGCToolWidget("", ui->sensorContents);
    tool->addUAS(mav);
    if (tool->loadSettings(autopilotdir.absolutePath() + "/general/calibration/calibration.qgw", false))
    {
        toolWidgets.append(tool);
        QGroupBox *box = new QGroupBox(ui->sensorContents);
        box->setTitle(tool->objectName());
        box->setLayout(new QVBoxLayout(box));
        box->layout()->addWidget(tool);
        ui->sensorLayout->addWidget(box);
    } else {
        delete tool;
    }

    // Load vehicle-specific autopilot configuration
    tool = new QGCToolWidget("", ui->sensorContents);
    tool->addUAS(mav);
    if (tool->loadSettings(autopilotdir.absolutePath() + "/" +  mav->getSystemTypeName().toLower() + "/calibration/calibration.qgw", false))
    {
        toolWidgets.append(tool);
        QGroupBox *box = new QGroupBox(ui->sensorContents);
        box->setTitle(tool->objectName());
        box->setLayout(new QVBoxLayout(box));
        box->layout()->addWidget(tool);
        ui->sensorLayout->addWidget(box);
    } else {
        delete tool;
    }

//    //description.txt
//    QFile sensortipsfile(autopilotdir.absolutePath() + "/general/calibration/description.txt");
//    sensortipsfile.open(QIODevice::ReadOnly);
////    ui->sensorTips->setHtml(sensortipsfile.readAll());
//    sensortipsfile.close();
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

void QGCPX4VehicleConfig::loadConfig()
{
    QGCToolWidget* tool;

    QDir autopilotdir(qApp->applicationDirPath() + "/files/" + mav->getAutopilotTypeName().toLower());
    QDir generaldir = QDir(autopilotdir.absolutePath() + "/general/widgets");
    QDir vehicledir = QDir(autopilotdir.absolutePath() + "/" + mav->getSystemTypeName().toLower() + "/widgets");
    if (!autopilotdir.exists("general"))
    {
     //TODO: Throw some kind of error here. There is no general configuration directory
        qWarning() << "Invalid general dir. no general configuration will be loaded.";
    }
    if (!autopilotdir.exists(mav->getAutopilotTypeName().toLower()))
    {
        //TODO: Throw an error here too, no autopilot specific configuration
        qWarning() << "Invalid vehicle dir, no vehicle specific configuration will be loaded.";
    }
    qDebug() << autopilotdir.absolutePath();
    qDebug() << generaldir.absolutePath();
    qDebug() << vehicledir.absolutePath();
    QFile xmlfile(autopilotdir.absolutePath() + "/arduplane.pdef.xml");
    if (xmlfile.exists() && !xmlfile.open(QIODevice::ReadOnly))
    {
        loadQgcConfig(false);
        doneLoadingConfig = true;
        return;
    }
    loadQgcConfig(true);

    QXmlStreamReader xml(xmlfile.readAll());
    xmlfile.close();

    //TODO: Testing to ensure that incorrectly formatted XML won't break this.
    while (!xml.atEnd())
    {
        if (xml.isStartElement() && xml.name() == "paramfile")
        {
            xml.readNext();
            while ((xml.name() != "paramfile") && !xml.atEnd())
            {
                QString valuetype = "";
                if (xml.isStartElement() && (xml.name() == "vehicles" || xml.name() == "libraries")) //Enter into the vehicles loop
                {
                    valuetype = xml.name().toString();
                    xml.readNext();
                    while ((xml.name() != valuetype) && !xml.atEnd())
                    {
                        if (xml.isStartElement() && xml.name() == "parameters") //This is a parameter block
                        {
                            QString parametersname = "";
                            if (xml.attributes().hasAttribute("name"))
                            {
                                    parametersname = xml.attributes().value("name").toString();
                            }
                            QVariantMap genset;
                            QVariantMap advset;

                            QString setname = parametersname;
                            xml.readNext();
                            int genarraycount = 0;
                            int advarraycount = 0;
                            while ((xml.name() != "parameters") && !xml.atEnd())
                            {
                                if (xml.isStartElement() && xml.name() == "param")
                                {
                                    QString humanname = xml.attributes().value("humanName").toString();
                                    QString name = xml.attributes().value("name").toString();
                                    QString tab= xml.attributes().value("user").toString();
                                    if (tab == "Advanced")
                                    {
                                        advset["title"] = parametersname;
                                    }
                                    else
                                    {
                                        genset["title"] = parametersname;
                                    }
                                    if (name.contains(":"))
                                    {
                                        name = name.split(":")[1];
                                    }
                                    QString docs = xml.attributes().value("documentation").toString();
                                    paramTooltips[name] = name + " - " + docs;

                                    int type = -1; //Type of item
                                    QMap<QString,QString> fieldmap;
                                    xml.readNext();
                                    while ((xml.name() != "param") && !xml.atEnd())
                                    {
                                        if (xml.isStartElement() && xml.name() == "values")
                                        {
                                            type = 1; //1 is a combobox
                                            if (tab == "Advanced")
                                            {
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "TYPE"] = "COMBO";
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_COMBOBOX_DESCRIPTION"] = humanname;
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_COMBOBOX_PARAMID"] = name;
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_COMBOBOX_COMPONENTID"] = 1;
                                            }
                                            else
                                            {
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "TYPE"] = "COMBO";
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_COMBOBOX_DESCRIPTION"] = humanname;
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_COMBOBOX_PARAMID"] = name;
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_COMBOBOX_COMPONENTID"] = 1;
                                            }
                                            int paramcount = 0;
                                            xml.readNext();
                                            while ((xml.name() != "values") && !xml.atEnd())
                                            {
                                                if (xml.isStartElement() && xml.name() == "value")
                                                {

                                                    QString code = xml.attributes().value("code").toString();
                                                    QString arg = xml.readElementText();
                                                    if (tab == "Advanced")
                                                    {
                                                        advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(paramcount) + "_TEXT"] = arg;
                                                        advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(paramcount) + "_VAL"] = code.toInt();
                                                    }
                                                    else
                                                    {
                                                        genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(paramcount) + "_TEXT"] = arg;
                                                        genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(paramcount) + "_VAL"] = code.toInt();
                                                    }
                                                    paramcount++;
                                                }
                                                xml.readNext();
                                            }
                                            if (tab == "Advanced")
                                            {
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_COMBOBOX_COUNT"] = paramcount;
                                            }
                                            else
                                            {
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_COMBOBOX_COUNT"] = paramcount;
                                            }
                                        }
                                        if (xml.isStartElement() && xml.name() == "field")
                                        {
                                            type = 2; //2 is a slider
                                            QString fieldtype = xml.attributes().value("name").toString();
                                            QString text = xml.readElementText();
                                            fieldmap[fieldtype] = text;
                                        }
                                        xml.readNext();
                                    }
                                    if (type == -1)
                                    {
                                        //Nothing inside! Assume it's a value, give it a default range.
                                        type = 2;
                                        QString fieldtype = "Range";
                                        QString text = "0 100"; //TODO: Determine a better way of figuring out default ranges.
                                        fieldmap[fieldtype] = text;
                                    }
                                    if (type == 2)
                                    {
                                        if (tab == "Advanced")
                                        {
                                            advset[setname + "\\" + QString::number(advarraycount) + "\\" + "TYPE"] = "SLIDER";
                                            advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_SLIDER_DESCRIPTION"] = humanname;
                                            advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_SLIDER_PARAMID"] = name;
                                            advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_SLIDER_COMPONENTID"] = 1;
                                        }
                                        else
                                        {
                                            genset[setname + "\\" + QString::number(genarraycount) + "\\" + "TYPE"] = "SLIDER";
                                            genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_SLIDER_DESCRIPTION"] = humanname;
                                            genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_SLIDER_PARAMID"] = name;
                                            genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_SLIDER_COMPONENTID"] = 1;
                                        }
                                        if (fieldmap.contains("Range"))
                                        {
                                            float min = 0;
                                            float max = 0;
                                            //Some range fields list "0-10" and some list "0 10". Handle both.
                                            if (fieldmap["Range"].split(" ").size() > 1)
                                            {
                                                min = fieldmap["Range"].split(" ")[0].trimmed().toFloat();
                                                max = fieldmap["Range"].split(" ")[1].trimmed().toFloat();
                                            }
                                            else if (fieldmap["Range"].split("-").size() > 1)
                                            {
                                                min = fieldmap["Range"].split("-")[0].trimmed().toFloat();
                                                max = fieldmap["Range"].split("-")[1].trimmed().toFloat();
                                            }
                                            if (tab == "Advanced")
                                            {
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_SLIDER_MIN"] = min;
                                                advset[setname + "\\" + QString::number(advarraycount) + "\\" + "QGC_PARAM_SLIDER_MAX"] = max;
                                            }
                                            else
                                            {
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_SLIDER_MIN"] = min;
                                                genset[setname + "\\" + QString::number(genarraycount) + "\\" + "QGC_PARAM_SLIDER_MAX"] = max;
                                            }
                                        }
                                    }
                                    if (tab == "Advanced")
                                    {
                                        advarraycount++;
                                        advset["count"] = advarraycount;
                                    }
                                    else
                                    {
                                        genarraycount++;
                                        genset["count"] = genarraycount;
                                    }
                                }
                                xml.readNext();
                            }
                            if (genarraycount > 0)
                            {
                                QWidget* parent = this;
                                if (valuetype == "vehicles")
                                {
                                    parent = ui->generalLeftContents;
                                }
                                else if (valuetype == "libraries")
                                {
                                    parent = ui->generalRightContents;
                                }
                                tool = new QGCToolWidget("", parent);
                                tool->addUAS(mav);
                                tool->setTitle(parametersname);
                                tool->setObjectName(parametersname);
                                tool->setSettings(genset);
                                QList<QString> paramlist = tool->getParamList();
                                for (int i=0;i<paramlist.size();i++)
                                {
                                    //Based on the airframe, we add the parameter to different categories.
                                    if (parametersname == "ArduPlane") //MAV_TYPE_FIXED_WING FIXED_WING
                                    {
                                        systemTypeToParamMap["FIXED_WING"].insert(paramlist[i],tool);
                                    }
                                    else if (parametersname == "ArduCopter") //MAV_TYPE_QUADROTOR "QUADROTOR
                                    {
                                        systemTypeToParamMap["QUADROTOR"].insert(paramlist[i],tool);
                                    }
                                    else if (parametersname == "APMrover2") //MAV_TYPE_GROUND_ROVER GROUND_ROVER
                                    {
                                        systemTypeToParamMap["GROUND_ROVER"].insert(paramlist[i],tool);
                                    }
                                    else
                                    {
                                        libParamToWidgetMap.insert(paramlist[i],tool);
                                    }
                                }

                                toolWidgets.append(tool);
                                QGroupBox *box = new QGroupBox(parent);
                                box->setTitle(tool->objectName());
                                box->setLayout(new QVBoxLayout(box));
                                box->layout()->addWidget(tool);
                                if (valuetype == "vehicles")
                                {
                                    ui->generalLeftLayout->addWidget(box);
                                }
                                else if (valuetype == "libraries")
                                {
                                    ui->generalRightLayout->addWidget(box);
                                }
                                box->hide();
                                toolToBoxMap[tool] = box;
                            }
                            if (advarraycount > 0)
                            {
                                QWidget* parent = this;
                                if (valuetype == "vehicles")
                                {
                                    parent = ui->generalLeftContents;
                                }
                                else if (valuetype == "libraries")
                                {
                                    parent = ui->generalRightContents;
                                }
                                tool = new QGCToolWidget("", parent);
                                tool->addUAS(mav);
                                tool->setTitle(parametersname);
                                tool->setObjectName(parametersname);
                                tool->setSettings(advset);
                                QList<QString> paramlist = tool->getParamList();
                                for (int i=0;i<paramlist.size();i++)
                                {
                                    //Based on the airframe, we add the parameter to different categories.
                                    if (parametersname == "ArduPlane") //MAV_TYPE_FIXED_WING FIXED_WING
                                    {
                                        systemTypeToParamMap["FIXED_WING"].insert(paramlist[i],tool);
                                    }
                                    else if (parametersname == "ArduCopter") //MAV_TYPE_QUADROTOR "QUADROTOR
                                    {
                                        systemTypeToParamMap["QUADROTOR"].insert(paramlist[i],tool);
                                    }
                                    else if (parametersname == "APMrover2") //MAV_TYPE_GROUND_ROVER GROUND_ROVER
                                    {
                                        systemTypeToParamMap["GROUND_ROVER"].insert(paramlist[i],tool);
                                    }
                                    else
                                    {
                                        libParamToWidgetMap.insert(paramlist[i],tool);
                                    }
                                }

                                toolWidgets.append(tool);
                                QGroupBox *box = new QGroupBox(parent);
                                box->setTitle(tool->objectName());
                                box->setLayout(new QVBoxLayout(box));
                                box->layout()->addWidget(tool);
                                if (valuetype == "vehicles")
                                {
                                    ui->generalLeftLayout->addWidget(box);
                                }
                                else if (valuetype == "libraries")
                                {
                                    ui->generalRightLayout->addWidget(box);
                                }
                                box->hide();
                                toolToBoxMap[tool] = box;
                            }
                        }
                        xml.readNext();
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }

    if (!paramTooltips.isEmpty()) {
           paramMgr->setParamDescriptions(paramTooltips);
    }
    doneLoadingConfig = true;
    //Config is finished, lets do a parameter request to ensure none are missed if someone else started requesting before we were finished.
    paramMgr->requestParameterListIfEmpty();
}

void QGCPX4VehicleConfig::setActiveUAS(UASInterface* active)
{
    // Hide items if NULL and abort
    if (!active) {
        ui->refreshButton->setEnabled(false);
        ui->refreshButton->show();
        ui->readButton->setEnabled(false);
        ui->readButton->show();
        ui->writeButton->setEnabled(false);
        ui->writeButton->show();
        ui->loadFileButton->setEnabled(false);
        ui->loadFileButton->show();
        ui->saveFileButton->setEnabled(false);
        ui->saveFileButton->show();

        return;
    }


    // Do nothing if UAS is already visible
    if (mav == active)
        return;

    if (mav)
    {

        // Disconnect old system
        disconnect(mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
                   SLOT(remoteControlChannelRawChanged(int,float)));
        //TODO use paramCommsMgr instead
        disconnect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
                   SLOT(parameterChanged(int,int,QString,QVariant)));
        disconnect(ui->refreshButton,SIGNAL(clicked()),
                   paramMgr,SLOT(requestParameterList()));

        // Delete all children from all fixed tabs.
        foreach(QWidget* child, ui->generalLeftContents->findChildren<QWidget*>()) {
            child->deleteLater();
        }
        foreach(QWidget* child, ui->generalRightContents->findChildren<QWidget*>()) {
            child->deleteLater();
        }
        foreach(QWidget* child, ui->advanceColumnContents->findChildren<QWidget*>()) {
            child->deleteLater();
        }
        foreach(QWidget* child, ui->sensorContents->findChildren<QWidget*>()) {
            child->deleteLater();
        }

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

    // Reset current state
    resetCalibrationRC();
    //TODO eliminate the separate RC_TYPE call
    mav->requestParameter(0, "RC_TYPE");

    chanCount = 0;

    //TODO get parameter changes via Param Mgr instead
    // Connect new system
    connect(mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
               SLOT(remoteControlChannelRawChanged(int,float)));
    connect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
               SLOT(parameterChanged(int,int,QString,QVariant)));
    connect(ui->refreshButton, SIGNAL(clicked()),
            paramMgr,SLOT(requestParameterList()));

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

    //Load configuration after 1ms. This allows it to go into the event loop, and prevents application hangups due to the
    //amount of time it actually takes to load the configuration windows.
    QTimer::singleShot(1,this,SLOT(loadConfig()));

    updateStatus(QString("Reading from system %1").arg(mav->getUASName()));

    // Since a system is now connected, enable the VehicleConfig UI.
    ui->refreshButton->setEnabled(true);
    ui->refreshButton->show();
    ui->readButton->setEnabled(true);
    ui->readButton->show();
    ui->writeButton->setEnabled(true);
    ui->writeButton->show();
    ui->loadFileButton->setEnabled(true);
    ui->loadFileButton->show();
    ui->saveFileButton->setEnabled(true);
    ui->saveFileButton->show();

    //TODO never true?
    if (mav->getAutopilotTypeName() == "ARDUPILOTMEGA") {
        ui->readButton->hide();
        ui->writeButton->hide();
    }
}

void QGCPX4VehicleConfig::resetCalibrationRC()
{
    for (unsigned int i = 0; i < chanMax; ++i)
    {
        rcMin[i] = 1500;
        rcMax[i] = 1500;
    }
}

/**
 * Sends the RC calibration to the vehicle and stores it in EEPROM
 */
void QGCPX4VehicleConfig::writeCalibrationRC()
{
    if (!mav) return;

    setTrimPositions();

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");

    // Do not write the RC type, as these values depend on this
    // active onboard parameter

    //TODO consolidate RC param sending in the UAS comms mgr
    for (unsigned int i = 0; i < chanCount; ++i)
    {
        //qDebug() << "SENDING" << minTpl.arg(i+1) << rcMin[i];
        mav->setParameter(0, minTpl.arg(i+1), rcMin[i]);
        QGC::SLEEP::usleep(50000);
        mav->setParameter(0, trimTpl.arg(i+1), rcTrim[i]);
        QGC::SLEEP::usleep(50000);
        mav->setParameter(0, maxTpl.arg(i+1), rcMax[i]);
        QGC::SLEEP::usleep(50000);
        mav->setParameter(0, revTpl.arg(i+1), (rcRev[i]) ? -1.0f : 1.0f);
        QGC::SLEEP::usleep(50000);
    }

    // Write mappings
    mav->setParameter(0, "RC_MAP_ROLL", (int32_t)(rcMapping[0]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_PITCH", (int32_t)(rcMapping[1]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_YAW", (int32_t)(rcMapping[2]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_THROTTLE", (int32_t)(rcMapping[3]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_MODE_SW", (int32_t)(rcMapping[4]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_AUX1", (int32_t)(rcMapping[5]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_AUX2", (int32_t)(rcMapping[6]+1));
    QGC::SLEEP::usleep(50000);
    mav->setParameter(0, "RC_MAP_AUX3", (int32_t)(rcMapping[7]+1));
    QGC::SLEEP::usleep(50000);
}

void QGCPX4VehicleConfig::requestCalibrationRC()
{
    paramMgr->requestRcCalibrationParamsUpdate();
}

void QGCPX4VehicleConfig::writeParameters()
{
    updateStatus(tr("Writing all onboard parameters."));
    writeCalibrationRC();
    mav->writeParametersToStorage();
}

void QGCPX4VehicleConfig::remoteControlChannelRawChanged(int chan, float val)
{
    // Check if index and values are sane
    if (chan < 0 || static_cast<unsigned int>(chan) >= chanMax || val < 500 || val > 2500)
        return;

    if (chan + 1 > (int)chanCount) {
        chanCount = chan+1;
    }

    // Raw value
    rcValue[chan] = val;

    // Update calibration data
    if (calibrationEnabled) {
        if (val < rcMin[chan]) {
            rcMin[chan] = val;
        }
        if (val > rcMax[chan]) {
            rcMax[chan] = val;
        }
    }

    if (channelWanted >= 0) {
        // If the first channel moved considerably, pick it
        if (fabsf(channelWantedList[chan] - val) > 300)
        {
            rcMapping[channelWanted] = chan;
            updateInvertedCheckboxes(chan);

            int chanFound = channelWanted;

            channelWanted = -1;

            // Confirm found channel
            QMessageBox msgBox;
            msgBox.setText(tr("%1 Channel found.").arg(channelNames[chanFound]));
            msgBox.setInformativeText(tr("Found %1 to be on the raw RC channel %2").arg(channelNames[chanFound]).arg(chan + 1));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            (void)msgBox.exec();
        }
    }

    // Find correct mapped channel
    for (int i = 0; i < chanCount; i++)
    {
        if (chan == rcMapping[i])
        {

            rcMappedValue[i] = (rcRev[chan]) ? rcMax[chan] - (val - rcMin[chan]) : val;

            // Copy min / max
            rcMappedMin[i] = rcMin[chan];
            rcMappedMax[i] = rcMax[chan];
        }
    }

    // Normalized value
    float normalized;

    if (val >= rcTrim[chan]) {
        normalized = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
    }
    else {
        normalized = -(rcTrim[chan] - val)/(rcTrim[chan] - rcMin[chan]);
    }

    // Bound
    normalized = qBound(-1.0f, normalized, 1.0f);
    // Invert
    normalized = (rcRev[chan]) ? -1.0f*normalized : normalized;

    if (chan == rcMapping[0]) {
        rcRoll = normalized;
    }
    else if (chan == rcMapping[1]) {
        rcPitch = normalized;
    }
    else if (chan == rcMapping[2]) {
        rcYaw = normalized;
    }
    else if (chan == rcMapping[3]) {
        if (rcRev[chan]) {
            rcThrottle = 1.0f + normalized;
        } else {
            rcThrottle = normalized;
        }

        rcThrottle = qBound(0.0f, rcThrottle, 1.0f);
    }
    else if (chan == rcMapping[4]) {
        rcMode = normalized;// MODE SWITCH
    }
    else if (chan == rcMapping[5]) {
        rcAux1 = normalized; // AUX1
    }
    else if (chan == rcMapping[6]) {
        rcAux2 = normalized;// AUX2
    }
    else if (chan == rcMapping[7]) {
        rcAux3 = normalized; // AUX3
    }

    dataModelChanged = true;

    //qDebug() << "RC CHAN:" << chan << "PPM:" << val << "NORMALIZED:" << normalized;
}

void QGCPX4VehicleConfig::updateInvertedCheckboxes(int index)
{
    unsigned int mapindex = rcMapping[index];

    switch (mapindex)
    {
    case 0:
        ui->invertCheckBox->setChecked(rcRev[index]);
        break;
    case 1:
        ui->invertCheckBox_2->setChecked(rcRev[index]);
        break;
    case 2:
        ui->invertCheckBox_3->setChecked(rcRev[index]);
        break;
    case 3:
        ui->invertCheckBox_4->setChecked(rcRev[index]);
        break;
    case 4:
        ui->invertCheckBox_5->setChecked(rcRev[index]);
        break;
    case 5:
        ui->invertCheckBox_6->setChecked(rcRev[index]);
        break;
    case 6:
        ui->invertCheckBox_7->setChecked(rcRev[index]);
        break;
    case 7:
        ui->invertCheckBox_8->setChecked(rcRev[index]);
        break;
    }
}

void QGCPX4VehicleConfig::handleRcParameterChange(QString parameterName, QVariant value)
{
    if (parameterName.startsWith("RC_")) {
        if (parameterName.startsWith("RC_MAP_")) {
            //RC Mapping radio channels to meaning
            // Order is: roll, pitch, yaw, throttle, mode sw, aux 1-3

            int intValue = value.toInt()  - 1;
            if (parameterName.startsWith("RC_MAP_ROLL")) {
                rcMapping[0] = intValue;
                ui->rollSpinBox->setValue(rcMapping[0]+1);
                ui->rollSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_PITCH")) {
                rcMapping[1] = intValue;
                ui->pitchSpinBox->setValue(rcMapping[1]+1);
                ui->pitchSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_YAW")) {
                rcMapping[2] = intValue;
                ui->yawSpinBox->setValue(rcMapping[2]+1);
                ui->yawSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_THROTTLE")) {
                rcMapping[3] = intValue;
                ui->throttleSpinBox->setValue(rcMapping[3]+1);
                ui->throttleSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_MODE_SW")) {
                rcMapping[4] = intValue;
                ui->modeSpinBox->setValue(rcMapping[4]+1);
                ui->modeSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_AUX1")) {
                rcMapping[5] = intValue;
                ui->aux1SpinBox->setValue(rcMapping[5]+1);
                ui->aux1SpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_AUX2")) {
                rcMapping[6] = intValue;
                ui->aux2SpinBox->setValue(rcMapping[6]+1);
                ui->aux2SpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_AUX3")) {
                rcMapping[7] = intValue;
                ui->aux3SpinBox->setValue(rcMapping[7]+1);
                ui->aux3SpinBox->setEnabled(true);
            }
        }
        else if (parameterName.startsWith("RC_SCALE_")) {
            // Scaling
            float floatVal = value.toFloat();
            if (parameterName.startsWith("RC_SCALE_ROLL")) {
                rcScaling[0] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_PITCH")) {
                rcScaling[1] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_YAW")) {
                rcScaling[2] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_THROTTLE")) {
                rcScaling[3] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_MODE_SW")) {
                rcScaling[4] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_AUX1")) {
                rcScaling[5] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_AUX2")) {
                rcScaling[6] = floatVal;
            }
            else if (parameterName.startsWith("RC_SCALE_AUX3")) {
                rcScaling[7] = floatVal;
            }
        }
        else if (parameterName.startsWith("RC_TYPE")) {
            if (0 != rcTypeUpdateRequested) {
                rcTypeUpdateRequested = 0;
                updateStatus(tr("Received RC type update, setting parameters based on model."));
                rcType = value.toInt();
                // Request all other parameters as well
                requestCalibrationRC();
            }
        }
    }
    else  {
        // Channel calibration values
        bool ok = false;
        unsigned int index = chanMax;
        QRegExp minTpl("RC?_MIN");
        minTpl.setPatternSyntax(QRegExp::Wildcard);
        QRegExp maxTpl("RC?_MAX");
        maxTpl.setPatternSyntax(QRegExp::Wildcard);
        QRegExp trimTpl("RC?_TRIM");
        trimTpl.setPatternSyntax(QRegExp::Wildcard);
        QRegExp revTpl("RC?_REV");
        revTpl.setPatternSyntax(QRegExp::Wildcard);

        // Do not write the RC type, as these values depend on this
        // active onboard parameter
        int intVal = value.toInt();

        if (minTpl.exactMatch(parameterName)) {
            index = parameterName.mid(2, 1).toInt(&ok) - 1;
            if (ok && index < chanMax) {
                rcMin[index] = intVal;
                updateRcWidgetValues();
            }
        }
        else if (maxTpl.exactMatch(parameterName)) {
            index = parameterName.mid(2, 1).toInt(&ok) - 1;
            if (ok && index < chanMax) {
                rcMax[index] = intVal;
                updateRcWidgetValues();
            }
        }
        else if (trimTpl.exactMatch(parameterName)) {
            index = parameterName.mid(2, 1).toInt(&ok) - 1;
            if (ok && index < chanMax) {
                rcTrim[index] = intVal;
            }
        }
        else if (revTpl.exactMatch(parameterName)) {
            index = parameterName.mid(2, 1).toInt(&ok) - 1;
            if (ok && index < chanMax) {
                rcRev[index] = (intVal == -1) ? true : false;
                updateInvertedCheckboxes(index);
            }
        }
    }

}

void QGCPX4VehicleConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (!doneLoadingConfig) {
        //We do not want to attempt to generate any UI elements until loading of the config file is complete.
        //We should re-request params later if needed, that is not implemented yet.
        return;
    }

    //TODO this may introduce a bug with param editor widgets not receiving param updates
    if (parameterName.startsWith("RC")) {
        handleRcParameterChange(parameterName,value);
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
        bool found = false;
        for (int i=0;i<toolWidgets.size();i++) {
            if (parameterName.startsWith(toolWidgets[i]->objectName())) {
                //It should be grouped with this one, add it.
                toolWidgets[i]->addParam(uas,component,parameterName,value);
                libParamToWidgetMap.insert(parameterName,toolWidgets[i]);
                found  = true;
                break;
            }
        }
        if (!found) {
            //New param type, create a QGroupBox for it.
            QWidget* parent = ui->advanceColumnContents;

            // Create the tool, attaching it to the QGroupBox
            QGCToolWidget *tool = new QGCToolWidget("", parent);
            QString tooltitle = parameterName;
            if (parameterName.split("_").size() > 1) {
                tooltitle = parameterName.split("_")[0] + "_";
            }
            tool->setTitle(tooltitle);
            tool->setObjectName(tooltitle);
            //tool->setSettings(set);
            libParamToWidgetMap.insert(parameterName,tool);
            toolWidgets.append(tool);
            tool->addParam(uas, component, parameterName, value);
            QGroupBox *box = new QGroupBox(parent);
            box->setTitle(tool->objectName());
            box->setLayout(new QVBoxLayout(box));
            box->layout()->addWidget(tool);

            libParamToWidgetMap.insert(parameterName,tool);
            toolWidgets.append(tool);
            ui->advancedColumnLayout->addWidget(box);

            toolToBoxMap[tool] = box;
        }
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


void QGCPX4VehicleConfig::setRCType(int type)
{
    if (!mav) return;

    // XXX TODO Add handling of RC_TYPE vs non-RC_TYPE here

    mav->setParameter(0, "RC_TYPE", type);
    rcTypeUpdateRequested = QGC::groundTimeMilliseconds();
    QTimer::singleShot(rcTypeTimeout+100, this, SLOT(checktimeOuts()));
}

void QGCPX4VehicleConfig::checktimeOuts()
{
    if (rcTypeUpdateRequested > 0)
    {
        if (QGC::groundTimeMilliseconds() - rcTypeUpdateRequested > rcTypeTimeout)
        {
            updateError(tr("Setting remote control timed out - is the system connected?"));
        }
    }
}


void QGCPX4VehicleConfig::updateRcWidgetValues()
{
    ui->rollWidget->setValueAndRange(rcMappedValue[0],rcMappedMin[0],rcMappedMax[0]);
    ui->pitchWidget->setValueAndRange(rcMappedValue[1],rcMappedMin[1],rcMappedMax[1]);
    ui->yawWidget->setValueAndRange(rcMappedValue[2],rcMappedMin[2],rcMappedMax[2]);
    ui->throttleWidget->setValueAndRange(rcMappedValue[3],rcMappedMin[3],rcMappedMax[3]);

    ui->radio5Widget->setValueAndRange(rcMappedValue[4],rcMin[4],rcMax[4]);
    ui->radio6Widget->setValueAndRange(rcMappedValue[5],rcMin[5],rcMax[5]);
    ui->radio7Widget->setValueAndRange(rcMappedValue[6],rcMin[6],rcMax[6]);
    ui->radio8Widget->setValueAndRange(rcMappedValue[7],rcMin[7],rcMax[7]);
}

void QGCPX4VehicleConfig::updateView()
{
    if (dataModelChanged) {
        dataModelChanged = false;

        updateRcWidgetValues();

        //update the channel labels
        ui->rollChanLabel->setText(QString("%1").arg(rcRoll, 5, 'f', 2, QChar(' ')));
        ui->pitchChanLabel->setText(QString("%1").arg(rcPitch, 5, 'f', 2, QChar(' ')));
        ui->yawChanLabel->setText(QString("%1").arg(rcYaw, 5, 'f', 2, QChar(' ')));
        ui->throttleChanLabel->setText(QString("%1").arg(rcThrottle, 5, 'f', 2, QChar(' ')));

        if (rcValue[rcMapping[4] != UINT16_MAX]) {
            ui->modeChanLabel->setText(QString("%1").arg(rcMode, 5, 'f', 2, QChar(' ')));
        } else {
            ui->modeChanLabel->setText(tr("---"));
        }

        if (rcValue[rcMapping[5]] != UINT16_MAX) {
            ui->aux1ChanLabel->setText(QString("%1").arg(rcAux1, 5, 'f', 2, QChar(' ')));
        } else {
            ui->aux1ChanLabel->setText(tr("---"));
        }

        if (rcValue[rcMapping[6]] != UINT16_MAX) {
            ui->aux2ChanLabel->setText(QString("%1").arg(rcAux2, 5, 'f', 2, QChar(' ')));
        } else {
            ui->aux2ChanLabel->setText(tr("---"));
        }

        if (rcValue[rcMapping[7]] != UINT16_MAX) {
            ui->aux3ChanLabel->setText(QString("%1").arg(rcAux3, 5, 'f', 2, QChar(' ')));
        } else {
            ui->aux3ChanLabel->setText(tr("---"));
        }

    }
}

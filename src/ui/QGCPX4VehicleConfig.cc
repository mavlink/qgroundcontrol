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
#include "QGCToolWidget.h"
#include "UASManager.h"
#include "LinkManager.h"
#include "UASParameterCommsMgr.h"
#include "ui_QGCPX4VehicleConfig.h"
#include "px4_configuration/QGCPX4AirframeConfig.h"
#include "px4_configuration/QGCPX4SensorCalibration.h"
#include "px4_configuration/PX4RCCalibration.h"

#ifdef QGC_QUPGRADE_ENABLED
#include <dialog_bare.h>
#endif

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
    
#ifdef QGC_QUPGRADE_ENABLED
    DialogBare *firmwareDialog = new DialogBare(this);
    ui->firmwareLayout->addWidget(firmwareDialog);

    connect(firmwareDialog, SIGNAL(connectLinks()), LinkManager::instance(), SLOT(connectAll()));
    connect(firmwareDialog, SIGNAL(disconnectLinks()), LinkManager::instance(), SLOT(disconnectAll()));
#else

    QLabel* label = new QLabel(this);
    label->setText("THIS VERSION OF QGROUNDCONTROL WAS BUILT WITHOUT QUPGRADE. To enable firmware upload support, checkout QUpgrade WITHIN the QGroundControl folder");
    ui->firmwareLayout->addWidget(label);
#endif

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

#if 0
void QGCPX4VehicleConfig::toggleSpektrumPairing(bool enabled)
{
    Q_UNUSED(enabled);
    
    if (!ui->dsm2RadioButton->isChecked() && !ui->dsmxRadioButton->isChecked()
            && !ui->dsmx8RadioButton->isChecked()) {
        // Reject
        QMessageBox warnMsgBox;
        warnMsgBox.setText(tr("Please select a Spektrum Protocol Version"));
        warnMsgBox.setInformativeText(tr("Please select either DSM2 or DSM-X\ndirectly below the pair button,\nbased on the receiver type."));
        warnMsgBox.setStandardButtons(QMessageBox::Ok);
        warnMsgBox.setDefaultButton(QMessageBox::Ok);
        (void)warnMsgBox.exec();
        return;
    }

    UASInterface* mav = UASManager::instance()->getActiveUAS();
    if (mav) {
        int rxSubType;
        if (ui->dsm2RadioButton->isChecked())
            rxSubType = 0;
        else if (ui->dsmxRadioButton->isChecked())
            rxSubType = 1;
        else // if (ui->dsmx8RadioButton->isChecked())
            rxSubType = 2;
        mav->pairRX(0, rxSubType);
    }
}
#endif

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
            tool = new QGCToolWidget("", "", parent);
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


     //TODO fix and reintegrate the Advanced parameter editor
//    // Generate widgets for the Advanced tab.
//    foreach (QString file,vehicledir.entryList(QDir::Files | QDir::NoDotAndDotDot))
//    {
//        if (file.toLower().endsWith(".qgw")) {
//            QWidget* parent = ui->advanceColumnContents;
//            tool = new QGCToolWidget("", parent);
//            if (tool->loadSettings(vehicledir.absoluteFilePath(file), false))
//            {
//                toolWidgets.append(tool);
//                QGroupBox *box = new QGroupBox(parent);
//                box->setTitle(tool->objectName());
//                box->setLayout(new QVBoxLayout(box));
//                box->layout()->addWidget(tool);
//                ui->advancedColumnLayout->addWidget(box);

//            } else {
//                delete tool;
//            }
//        }
//    }


    // Load tabs for general configuration
    foreach (QString dir,generaldir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QPushButton *button = new QPushButton(this);
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
                tool = new QGCToolWidget("", "", tab);
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
        QPushButton *button = new QPushButton(this);
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
                tool = new QGCToolWidget("", "", tab);
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
                                tool = new QGCToolWidget(parametersname, parametersname, parent);
                                tool->addUAS(mav);
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
                                tool = new QGCToolWidget(parametersname, parametersname, parent);
                                tool->addUAS(mav);
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
//        foreach(QWidget* child, ui->sensorLayout->findChildren<QWidget*>()) {
//            child->deleteLater();
//        }

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

    //Load configuration after 1ms. This allows it to go into the event loop, and prevents application hangups due to the
    //amount of time it actually takes to load the configuration windows.
    QTimer::singleShot(1,this,SLOT(loadConfig()));

    updateStatus(QString("Reading from system %1").arg(mav->getUASName()));

    // Since a system is now connected, enable the VehicleConfig UI.
    // Enable buttons
    ui->advancedMenuButton->setEnabled(true);
    ui->airframeMenuButton->setEnabled(true);
    ui->sensorMenuButton->setEnabled(true);
    ui->rcMenuButton->setEnabled(true);
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

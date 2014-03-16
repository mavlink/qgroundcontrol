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
    chanCount(0),
    channelWanted(-1),
    channelReverseStateWanted(-1),
    rcRoll(0.0f),
    rcPitch(0.0f),
    rcYaw(0.0f),
    rcThrottle(0.0f),
    rcMode(0.0f),
    rcAssist(0.0f),
    rcMission(0.0f),
    rcReturn(0.0f),
    rcFlaps(0.0f),
    rcAux1(0.0f),
    rcAux2(0.0f),
    dataModelChanged(true),
    calibrationEnabled(false),
    configEnabled(false),
    px4AirframeConfig(NULL),
    planeBack(":/files/images/px4/rc/cessna_back.png"),
    planeSide(":/files/images/px4/rc/cessna_side.png"),
    px4SensorCalibration(NULL),
    ui(new Ui::QGCPX4VehicleConfig)
{
    doneLoadingConfig = false;

    channelNames << "Roll / Aileron";
    channelNames << "Pitch / Elevator";
    channelNames << "Yaw / Rudder";
    channelNames << "Throttle";
    channelNames << "Main Mode Switch";
    channelNames << "Assist Switch";
    channelNames << "Mission Switch";
    channelNames << "Return Switch";
    channelNames << "Flaps";
    channelNames << "Aux1";
    channelNames << "Aux2";
    channelNames << "Aux3";
    channelNames << "Aux4";
    channelNames << "Aux5";
    channelNames << "Aux6";
    channelNames << "Aux7";
    channelNames << "Aux8";

    setObjectName("QGC_VEHICLECONFIG");
    ui->setupUi(this);

    ui->advancedMenuButton->setEnabled(false);
    ui->airframeMenuButton->setEnabled(false);
    ui->sensorMenuButton->setEnabled(false);
    ui->rcMenuButton->setEnabled(false);
    ui->generalMenuButton->hide();

    px4AirframeConfig = new QGCPX4AirframeConfig(this);
    ui->airframeLayout->addWidget(px4AirframeConfig);

    px4SensorCalibration = new QGCPX4SensorCalibration(this);
    ui->sensorLayout->addWidget(px4SensorCalibration);

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
    ui->radio9Widget->setOrientation(Qt::Horizontal);
    ui->radio9Widget->setName("Radio 9");
    ui->radio10Widget->setOrientation(Qt::Horizontal);
    ui->radio10Widget->setName("Radio 10");
    ui->radio11Widget->setOrientation(Qt::Horizontal);
    ui->radio11Widget->setName("Radio 11");
    ui->radio12Widget->setOrientation(Qt::Horizontal);
    ui->radio12Widget->setName("Radio 12");
    ui->radio13Widget->setOrientation(Qt::Horizontal);
    ui->radio13Widget->setName("Radio 13");
    ui->radio14Widget->setOrientation(Qt::Horizontal);
    ui->radio14Widget->setName("Radio 14");
    ui->radio15Widget->setOrientation(Qt::Horizontal);
    ui->radio15Widget->setName("Radio 15");
    ui->radio16Widget->setOrientation(Qt::Horizontal);
    ui->radio16Widget->setName("Radio 16");
    ui->radio17Widget->setOrientation(Qt::Horizontal);
    ui->radio17Widget->setName("Radio 17");
    ui->radio18Widget->setOrientation(Qt::Horizontal);
    ui->radio18Widget->setName("Radio 18");

    connect(ui->rcMenuButton,SIGNAL(clicked()),
            this,SLOT(rcMenuButtonClicked()));
    connect(ui->rcCopyTrimButton, SIGNAL(clicked()),
            this, SLOT(copyAttitudeTrim()));
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

    connect(ui->advancedCheckBox, SIGNAL(clicked(bool)), ui->advancedGroupBox, SLOT(setVisible(bool)));
    ui->advancedGroupBox->setVisible(false);

#if 0
    // XXX WIP don't connect signal until completed, otherwise view will show after advanced is turned on and then off
    connect(ui->advancedCheckBox, SIGNAL(clicked(bool)), ui->graphicsView, SLOT(setHidden(bool)));
    ui->graphicsView->setVisible(true);
    ui->graphicsView->setScene(&scene);

    scene.addPixmap(planeBack);
    scene.addPixmap(planeSide);
#else
    // XXX hide while WIP
    ui->graphicsView->hide();
#endif

    ui->rcCalibrationButton->setCheckable(true);
    ui->rcCalibrationButton->setEnabled(false);
    connect(ui->rcCalibrationButton, SIGNAL(clicked(bool)), this, SLOT(toggleCalibrationRC(bool)));
    ui->spektrumPairButton->setCheckable(false);
    ui->spektrumPairButton->setEnabled(false);
    connect(ui->spektrumPairButton, SIGNAL(clicked(bool)), this, SLOT(toggleSpektrumPairing(bool)));

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
    connect(ui->assistSwSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAssistChan(int)));
    connect(ui->missionSwSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setMissionChan(int)));
    connect(ui->returnSwSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setReturnChan(int)));
    connect(ui->flapsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setFlapsChan(int)));
    connect(ui->aux1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAux1Chan(int)));
    connect(ui->aux2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(setAux2Chan(int)));

    // Connect RC reverse assignments
    connect(ui->invertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setRollInverted(bool)));
    connect(ui->invertCheckBox_2, SIGNAL(clicked(bool)), this, SLOT(setPitchInverted(bool)));
    connect(ui->invertCheckBox_3, SIGNAL(clicked(bool)), this, SLOT(setYawInverted(bool)));
    connect(ui->invertCheckBox_4, SIGNAL(clicked(bool)), this, SLOT(setThrottleInverted(bool)));
    connect(ui->invertCheckBox_5, SIGNAL(clicked(bool)), this, SLOT(setModeInverted(bool)));
    connect(ui->assistSwInvertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setAssistInverted(bool)));
    connect(ui->missionSwInvertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setMissionInverted(bool)));
    connect(ui->returnSwInvertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setReturnInverted(bool)));
    connect(ui->flapsInvertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setFlapsInverted(bool)));
    connect(ui->aux1InvertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setAux1Inverted(bool)));
    connect(ui->aux2InvertCheckBox, SIGNAL(clicked(bool)), this, SLOT(setAux2Inverted(bool)));

    connect(ui->rollButton, SIGNAL(clicked()), this, SLOT(identifyRollChannel()));
    connect(ui->pitchButton, SIGNAL(clicked()), this, SLOT(identifyPitchChannel()));
    connect(ui->yawButton, SIGNAL(clicked()), this, SLOT(identifyYawChannel()));
    connect(ui->throttleButton, SIGNAL(clicked()), this, SLOT(identifyThrottleChannel()));
    connect(ui->modeButton, SIGNAL(clicked()), this, SLOT(identifyModeChannel()));
    connect(ui->assistSwButton, SIGNAL(clicked()), this, SLOT(identifyAssistChannel()));
    connect(ui->missionSwButton, SIGNAL(clicked()), this, SLOT(identifyMissionChannel()));
    connect(ui->returnSwButton, SIGNAL(clicked()), this, SLOT(identifyReturnChannel()));
    connect(ui->flapsButton, SIGNAL(clicked()), this, SLOT(identifyFlapsChannel()));
    connect(ui->aux1Button, SIGNAL(clicked()), this, SLOT(identifyAux1Channel()));
    connect(ui->aux2Button, SIGNAL(clicked()), this, SLOT(identifyAux2Channel()));
    connect(ui->persistRcValuesButt,SIGNAL(clicked()), this, SLOT(writeCalibrationRC()));

    //set rc values to defaults
    for (unsigned int i = 0; i < chanMax; i++) {
        rcValue[i] = UINT16_MAX;
        rcValueReversed[i] = UINT16_MAX;
        rcMapping[i] = i;
        rcToFunctionMapping[i] = i;
        channelWantedList[i] = (float)UINT16_MAX;//TODO need to clean these up!
        rcMin[i] = 1000.0f;
        rcMax[i] = 2000.0f;

        // Mapping not established here, so can't pick values via mapping yet!
        rcMappedMin[i] = 1000;
        rcMappedMax[i] = 2000;
        rcMappedValue[i] = UINT16_MAX;
        rcMappedValueRev[i] = UINT16_MAX;
        rcMappedNormalizedValue[i] = 0.0f;
    }

    for (unsigned int i = chanMax -1; i < chanMappedMax; i++) {
        rcMapping[i] = -1;
        rcMappedMin[i] = 1000;
        rcMappedMax[i] = 2000;
        rcMappedValue[i] = UINT16_MAX;
        rcMappedValueRev[i] = UINT16_MAX;
        rcMappedNormalizedValue[i] = 0.0f;
    }

    firmwareMenuButtonClicked();

    updateTimer.setInterval(150);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateView()));
    updateTimer.start();

    ui->rcLabel->setText(tr("NO RADIO CONTROL INPUT DETECTED. PLEASE ENSURE THE TRANSMITTER IS ON."));

}

QGCPX4VehicleConfig::~QGCPX4VehicleConfig()
{
    delete ui;
}

void QGCPX4VehicleConfig::rcMenuButtonClicked()
{
    //TODO eg ui->stackedWidget->findChild("rcConfig");
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_RC);
    ui->tabTitleLabel->setText(tr("Radio Calibration"));
}

void QGCPX4VehicleConfig::sensorMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_SENSOR_CAL);
    ui->tabTitleLabel->setText(tr("Sensor Calibration"));
}

void QGCPX4VehicleConfig::generalMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_GENERAL_CONFIG);
    ui->tabTitleLabel->setText(tr("General Configuration Options"));
}

void QGCPX4VehicleConfig::advancedMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_ADV_CONFIG);
    ui->tabTitleLabel->setText(tr("Advanced Configuration Options"));
}

void QGCPX4VehicleConfig::airframeMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_AIRFRAME_CONFIG);
    ui->tabTitleLabel->setText(tr("Airframe Configuration"));
}

void QGCPX4VehicleConfig::firmwareMenuButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(WIDGET_INDEX_FIRMWARE);
    ui->tabTitleLabel->setText(tr("Firmware Upgrade"));
}

void QGCPX4VehicleConfig::identifyChannelMapping(int aert_index)
{
    if (chanCount == 0 || aert_index < 0)
        return;

    int oldmapping = rcMapping[aert_index];
    channelWanted = aert_index;

    for (unsigned i = 0; i < chanMax; i++) {
        if (i >= chanCount) {
            channelWantedList[i] = 0;
        }
        else {
            channelWantedList[i] = rcValue[i];
        }
    }

    msgBox.setText(tr("Detecting %1 ...\t\t").arg(channelNames[channelWanted]));
    msgBox.setInformativeText(tr("Please move stick, switch or potentiometer for this channel all the way up/down or left/right."));
    msgBox.setStandardButtons(QMessageBox::NoButton);
    skipActionButton = msgBox.addButton(tr("Skip"),QMessageBox::RejectRole);
    msgBox.exec();
    skipActionButton->hide();
    msgBox.removeButton(skipActionButton);
    if (msgBox.clickedButton() == skipActionButton ){
        channelWanted = -1;
        rcMapping[aert_index] = oldmapping;
    }
    skipActionButton = NULL;

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

void QGCPX4VehicleConfig::copyAttitudeTrim() {
    if (configEnabled) {

        QMessageBox warnMsgBox;
        warnMsgBox.setText(tr("Attitude trim denied during RC calibration"));
        warnMsgBox.setInformativeText(tr("Please end the RC calibration before doing attitude trim."));
        warnMsgBox.setStandardButtons(QMessageBox::Ok);
        warnMsgBox.setDefaultButton(QMessageBox::Ok);
        (void)warnMsgBox.exec();
    }

    // Not aborted, but warn user

    msgBox.setText(tr("Confirm Attitude Trim"));
    msgBox.setInformativeText(tr("On clicking OK, the current Roll / Pitch / Yaw stick positions will be set as trim values in auto flight. Do NOT reset your trim values after this step."));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);//allow user to cancel upload after reviewing values
    int msgBoxResult = msgBox.exec();
    if (QMessageBox::Cancel == msgBoxResult) {
        return;
        // do not execute
    }

    mav->startRadioControlCalibration(2);
    QGC::SLEEP::msleep(100);
    mav->endRadioControlCalibration();
}

void QGCPX4VehicleConfig::setTrimPositions()
{
    int rollMap = rcMapping[0];
    int pitchMap = rcMapping[1];
    int yawMap = rcMapping[2];
    int throttleMap = rcMapping[3];

    // Reset all trims, as some might not be touched
    for (unsigned i = 0; i < chanCount; i++) {
        rcTrim[i] = 1500;
    }

    bool throttleDone = false;

    while (!throttleDone) {
        // Set trim to min if stick is close to min
        if (abs(rcValue[throttleMap] - rcMin[throttleMap]) < 100) {
            rcTrim[throttleMap] = rcMin[throttleMap];   // throttle
            throttleDone = true;
        }
        // Set trim to max if stick is close to max
        else if (abs(rcValue[throttleMap] - rcMax[throttleMap]) < 100) {
            rcTrim[throttleMap] = rcMax[throttleMap];   // throttle
            throttleDone = true;
        }
        else
        {
            // Reject
            QMessageBox warnMsgBox;
            warnMsgBox.setText(tr("Throttle Stick Trim Position Invalid"));
            warnMsgBox.setInformativeText(tr("The throttle stick is not in the min position. Please set it to the zero throttle position and then click OK."));
            warnMsgBox.setStandardButtons(QMessageBox::Ok);
            warnMsgBox.setDefaultButton(QMessageBox::Ok);
            (void)warnMsgBox.exec();
            // wait long enough to get some data
            QGC::SLEEP::msleep(500);
        }
    }

    // Set trim for roll, pitch, yaw, throttle
    rcTrim[rollMap] = rcValue[rollMap]; // roll
    rcTrim[pitchMap] = rcValue[pitchMap]; // pitch
    rcTrim[yawMap] = rcValue[yawMap]; // yaw

    // Mode switch and optional modes, might not be mapped (== -1)
    for (unsigned i = 4; i < chanMappedMax; i++) {
        if (rcMapping[i] >= 0 && rcMapping[i] < (int)chanCount) {
            rcTrim[rcMapping[i]] = ((rcMax[rcMapping[i]] - rcMin[rcMapping[i]]) / 2.0f) + rcMin[rcMapping[i]];
        } else if (rcMapping[i] != -1){
            qDebug() << "RC MAPPING FAILED #" << i << "VAL:" << rcMapping[i];
        }
    }
}

void QGCPX4VehicleConfig::detectChannelInversion(int aert_index)
{
    if (chanCount == 0 || aert_index < 0 || aert_index >= (int)chanMappedMax)
        return;

    bool oldstatus = rcRev[rcMapping[aert_index]];
    channelReverseStateWanted = aert_index;

    // Reset search list
    for (unsigned i = 0; i < chanMax; i++) {
        if (i >= chanCount) {
            channelReverseStateWantedList[i] = 0;
        }
        else {
            channelReverseStateWantedList[i] = rcValue[i];
        }
    }

    QStringList instructions;
    instructions << "ROLL: Move stick left";
    instructions << "PITCH: Move stick down";//matches the other sticks: should cause DECREASE in raw rc channel value when not reversed
    instructions << "YAW: Move stick left";
    instructions << "THROTTLE: Move stick down";
    instructions << "MODE SWITCH: Push down / towards you";
    instructions << "ASSISTED SWITCH: Push down / towards you";
    instructions << "MISSION SWITCH: Push down / towards you";
    instructions << "RETURN SWITCH: Push down / towards you";
    instructions << "FLAPS: Push down / towards you or turn dial to the leftmost position";
    instructions << "AUX1: Push down / towards you or turn dial to the leftmost position";
    instructions << "AUX2: Push down / towards you or turn dial to the leftmost position";

    msgBox.setText(tr("%1 Direction").arg(channelNames[channelReverseStateWanted]));
    msgBox.setInformativeText(tr("%2").arg((aert_index < instructions.length()) ? instructions[aert_index] : ""));
    msgBox.setStandardButtons(QMessageBox::NoButton);
    skipActionButton = msgBox.addButton(tr("Skip"),QMessageBox::RejectRole);
    msgBox.exec();
    skipActionButton->hide();
    msgBox.removeButton(skipActionButton);
    if (msgBox.clickedButton() == skipActionButton ){
        channelReverseStateWanted = -1;
        rcRev[rcMapping[aert_index]] = oldstatus;
    }
    skipActionButton = NULL;
}

void QGCPX4VehicleConfig::startCalibrationRC()
{
    if (chanCount < 5 && !mav) {
        QMessageBox::warning(0,
                             tr("RC not Connected"),
                             tr("Is the RC receiver connected and transmitter turned on? Detected %1 radio channels. To operate PX4, you need at least 5 channels. ").arg(chanCount));
        ui->rcCalibrationButton->setChecked(false);
        return;
    }

    // XXX magic number: Set to 1 for radio input disable
    mav->startRadioControlCalibration(1);

    // reset all channel mappings above Ch 5 to invalid/unused value before starting calibration
    for (unsigned int j= 5; j < chanMappedMax; j++) {
        rcMapping[j] = -1;
    }

    configEnabled = true;

    QMessageBox::warning(0,tr("Safety Warning"),
                         tr("Starting RC calibration.\n\nEnsure that motor power is disconnected, all props are removed, RC transmitter and receiver are powered and connected.\n\nReset transmitter trims to center, then click OK to continue"));

    //go ahead and try to map first 8 channels, now that user can skip channels
    for (int i = 0; i < 8; i++) {
        identifyChannelMapping(i);
    }

    //QMessageBox::information(0,"Information","Additional channels have not been mapped, but can be mapped in the channel table below.");
    configEnabled = false;
    ui->rcCalibrationButton->setText(tr("Finish RC Calibration"));
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
    ui->radio9Widget->showMinMax();
    ui->radio10Widget->showMinMax();
    ui->radio11Widget->showMinMax();
    ui->radio12Widget->showMinMax();
    ui->radio13Widget->showMinMax();
    ui->radio14Widget->showMinMax();
    ui->radio15Widget->showMinMax();
    ui->radio16Widget->showMinMax();
    ui->radio17Widget->showMinMax();
    ui->radio18Widget->showMinMax();

    msgBox.setText(tr("Information"));
    msgBox.setInformativeText(tr("Please move the sticks to their extreme positions, including all switches. Then click on the OK button once finished"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.show();
    msgBox.move((frameGeometry().width() - msgBox.width()) / 4.0f,(frameGeometry().height() - msgBox.height()) / 1.5f);
    int msgBoxResult = msgBox.exec();
    if (QMessageBox::Ok == msgBoxResult) {
        stopCalibrationRC();
    }
}

void QGCPX4VehicleConfig::stopCalibrationRC()
{
    if (!calibrationEnabled)
        return;

    // Try to identify inverted channels, but only for R/P/Y/T
    for (int i = 0; i < 4; i++) {
        detectChannelInversion(i);
    }

    QMessageBox::information(0,"Trims","Ensure THROTTLE is in the LOWEST position and roll / pitch / yaw are CENTERED. Click OK to continue");

    calibrationEnabled = false;
    configEnabled = false;
    ui->rcCalibrationButton->setText(tr("Start RC Calibration"));
    ui->rcCalibrationButton->blockSignals(true);
    ui->rcCalibrationButton->setChecked(false);
    ui->rcCalibrationButton->blockSignals(false);

    ui->rollWidget->hideMinMax();
    ui->pitchWidget->hideMinMax();
    ui->yawWidget->hideMinMax();
    ui->throttleWidget->hideMinMax();
    ui->radio5Widget->hideMinMax();
    ui->radio6Widget->hideMinMax();
    ui->radio7Widget->hideMinMax();
    ui->radio8Widget->hideMinMax();
    ui->radio9Widget->hideMinMax();
    ui->radio10Widget->hideMinMax();
    ui->radio11Widget->hideMinMax();
    ui->radio12Widget->hideMinMax();
    ui->radio13Widget->hideMinMax();
    ui->radio14Widget->hideMinMax();
    ui->radio15Widget->hideMinMax();
    ui->radio16Widget->hideMinMax();
    ui->radio17Widget->hideMinMax();
    ui->radio18Widget->hideMinMax();

    for (unsigned int i = 0; i < chanCount; i++) {
        if (rcMin[i] > 1350) {
            rcMin[i] = 1000;
        }

        if (rcMax[i] < 1650) {
            rcMax[i] = 2000;
        }
    }

    qDebug() << "SETTING TRIM";
    setTrimPositions();

    QString statusstr = tr("The calibration has been finished. Please click OK to upload it to the autopilot.");
//    statusstr = tr("This is the RC calibration information that will be sent to the autopilot if you click OK. To prevent transmission, click Cancel.");
//    statusstr += tr("  Normal values range from 1000 to 2000, with disconnected channels reading 1000, 1500, 2000\n\n");
//    statusstr += tr("Channel\tMin\tCenter\tMax\n");
//    statusstr += "-------\t---\t------\t---\n";
//    for (unsigned int i=0; i < chanCount; i++) {
//        statusstr += QString::number(i) +"\t"+ QString::number(rcMin[i]) +"\t"+ QString::number(rcValue[i]) +"\t"+ QString::number(rcMax[i]) +"\n";
//    }

    msgBox.setText(tr("Confirm Calibration"));
    msgBox.setInformativeText(statusstr);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);//allow user to cancel upload after reviewing values
    int msgBoxResult = msgBox.exec();

    // Done, exit calibration mode now
    mav->endRadioControlCalibration();

    if (QMessageBox::Cancel == msgBoxResult) {
        return; //don't commit these values
    } else {
        QMessageBox::information(0,"Uploading the RC Calibration","The configuration will now be uploaded and permanently stored.");
        writeCalibrationRC();
    }

    // Read calibration back to update widget states and validate
    paramMgr->requestParameterList();
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

        // Disconnect old system
        disconnect(mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
                   SLOT(remoteControlChannelRawChanged(int,float)));
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

    // Reset current state
    resetCalibrationRC();
    //TODO eliminate the separate RC_TYPE call
    mav->requestParameter(0, "RC_TYPE");

    chanCount = 0;

    // Connect new system
    connect(mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
               SLOT(remoteControlChannelRawChanged(int,float)));
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

    ui->rcCalibrationButton->setEnabled(true);
    ui->spektrumPairButton->setEnabled(true);
}

void QGCPX4VehicleConfig::resetCalibrationRC()
{
    for (unsigned int i = 0; i < chanMax; ++i) {
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

    updateStatus(tr("Sending RC configuration and storing to persistent memory."));

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");

    // Do not write the RC type, as these values depend on this
    // active onboard parameter

    for (unsigned int i = 0; i < chanCount; ++i) {
        //qDebug() << "SENDING" << minTpl.arg(i+1) << rcMin[i];
        paramMgr->setPendingParam(0, minTpl.arg(i+1), rcMin[i]);
        paramMgr->setPendingParam(0, trimTpl.arg(i+1), rcTrim[i]);
        paramMgr->setPendingParam(0, maxTpl.arg(i+1), rcMax[i]);
        paramMgr->setPendingParam(0, revTpl.arg(i+1), (rcRev[i]) ? -1.0f : 1.0f);
    }

    // Write mappings
    paramMgr->setPendingParam(0, "RC_MAP_ROLL", (int32_t)(rcMapping[0]+1));
    paramMgr->setPendingParam(0, "RC_MAP_PITCH", (int32_t)(rcMapping[1]+1));
    paramMgr->setPendingParam(0, "RC_MAP_YAW", (int32_t)(rcMapping[2]+1));
    paramMgr->setPendingParam(0, "RC_MAP_THROTTLE", (int32_t)(rcMapping[3]+1));
    paramMgr->setPendingParam(0, "RC_MAP_MODE_SW", (int32_t)(rcMapping[4]+1));
    paramMgr->setPendingParam(0, "RC_MAP_ASSIST_SW", (int32_t)(rcMapping[5]+1));
    paramMgr->setPendingParam(0, "RC_MAP_MISSIO_SW", (int32_t)(rcMapping[6]+1));
    paramMgr->setPendingParam(0, "RC_MAP_RETURN_SW", (int32_t)(rcMapping[7]+1));
    paramMgr->setPendingParam(0, "RC_MAP_FLAPS", (int32_t)(rcMapping[8]+1));
    paramMgr->setPendingParam(0, "RC_MAP_AUX1", (int32_t)(rcMapping[9]+1));
    paramMgr->setPendingParam(0, "RC_MAP_AUX2", (int32_t)(rcMapping[10]+1));

    //let the param mgr manage sending all the pending RC_foo updates and persisting after
    paramMgr->sendPendingParameters(true);

}

void QGCPX4VehicleConfig::requestCalibrationRC()
{
    paramMgr->requestRcCalibrationParamsUpdate();
}

void QGCPX4VehicleConfig::writeParameters()
{
    updateStatus(tr("Writing all onboard parameters."));
    writeCalibrationRC();
}

void QGCPX4VehicleConfig::remoteControlChannelRawChanged(int chan, float fval)
{
    // Check if index and values are sane
    if (chan < 0 || static_cast<unsigned int>(chan) >= chanMax || fval < 500.0f || fval > 2500.0f)
        return;

    if (chan + 1 > (int)chanCount) {
        chanCount = chan+1;
    }

    // Raw value
    float deltaRaw = fabsf(fval - rcValue[chan]);
    float delta = fabsf(fval - rcMappedValue[rcToFunctionMapping[chan]]);
    if (!configEnabled && !calibrationEnabled &&
        (deltaRaw < 12.0f && delta < 12.0f && rcValue[chan] > 800 && rcValue[chan] < 2200))
    {
        // ignore tiny jitter values
        return;
    }
    else {
        rcValue[chan] = fval;
    }


    // Update calibration data
    if (calibrationEnabled) {
        if (fval < rcMin[chan]) {
            rcMin[chan] = fval;
        }
        if (fval > rcMax[chan]) {
            rcMax[chan] = fval;
        }
    }

    if (channelWanted >= 0) {
        // If the first channel moved considerably, pick it
        if (fabsf(channelWantedList[chan] - fval) > 300.0f) {
            rcMapping[channelWanted] = chan;
            updateMappingView(channelWanted);

            int chanFound = channelWanted;
            channelWanted = -1;

            // Confirm found channel
            msgBox.setText(tr("Found %1 \t\t").arg(channelNames[chanFound]));
            msgBox.setInformativeText(tr("Assigned raw RC channel %2").arg(chan + 1));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            skipActionButton->hide();
            msgBox.removeButton(skipActionButton);

            (void)msgBox.exec();

            // XXX fuse with parameter update handling
            switch (chanFound) {
            case 0:
                ui->rollSpinBox->setValue(chan + 1);
                break;
            case 1:
                ui->pitchSpinBox->setValue(chan + 1);
                break;
            case 2:
                ui->yawSpinBox->setValue(chan + 1);
                break;
            case 3:
                ui->throttleSpinBox->setValue(chan + 1);
                break;
            case 4:
                ui->modeSpinBox->setValue(chan + 1);
                break;
            case 5:
                ui->assistSwSpinBox->setValue(chan + 1);
                break;
            case 6:
                ui->missionSwSpinBox->setValue(chan + 1);
                break;
            case 7:
                ui->returnSwSpinBox->setValue(chan + 1);
                break;
            case 8:
                ui->flapsSpinBox->setValue(chan + 1);
                break;
            case 9:
                ui->aux1SpinBox->setValue(chan + 1);
                break;
            case 10:
                ui->aux2SpinBox->setValue(chan + 1);
                break;
            }
        }
    }

    // Reverse raw value
    rcValueReversed[chan] = (rcRev[chan]) ? rcMax[chan] - (fval - rcMin[chan]) : fval;

    // Normalized value
    float normalized;
    float chanTrim = rcTrim[chan];
    if (fval >= rcTrim[chan]) {
        normalized = (fval - chanTrim)/(rcMax[chan] - chanTrim);
    }
    else {
        normalized = -(chanTrim - fval)/(chanTrim - rcMin[chan]);
    }

    // Bound
    normalized = qBound(-1.0f, normalized, 1.0f);
    // Invert
    normalized = (rcRev[chan]) ? -1.0f*normalized : normalized;

    // Find correct mapped channel
    rcMappedValueRev[rcToFunctionMapping[chan]] = rcValueReversed[chan];
    rcMappedValue[rcToFunctionMapping[chan]] = fval;

    // Copy min / max
    rcMappedMin[rcToFunctionMapping[chan]] = rcMin[chan];
    rcMappedMax[rcToFunctionMapping[chan]] = rcMax[chan];
    rcMappedNormalizedValue[rcToFunctionMapping[chan]] = normalized;

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
        rcThrottle = normalized;
//        if (rcRev[chan]) {
//            rcThrottle = 1.0f + normalized;
//        }
//        else {
//            rcThrottle = normalized;
//        }

//        rcThrottle = qBound(0.0f, rcThrottle, 1.0f);
    }
    else if (chan == rcMapping[4]) {
        rcMode = normalized; // MODE SWITCH
    }
    else if (chan == rcMapping[5]) {
        rcAssist = normalized; // ASSIST SWITCH
    }
    else if (chan == rcMapping[6]) {
        rcMission = normalized; // MISSION SWITCH
    }
    else if (chan == rcMapping[7]) {
        rcReturn = normalized; // RETURN SWITCH
    }
    else if (chan == rcMapping[8]) {
        rcFlaps = normalized; // FLAPS
    }
    else if (chan == rcMapping[9]) {
        rcAux1 = normalized; // AUX2
    }
    else if (chan == rcMapping[10]) {
        rcAux2 = normalized; // AUX3
    }

    if (channelReverseStateWanted >= 0) {
        // If the *right* channel moved considerably, evaluate it
        if (fabsf(fval - 1500) > 350.0f &&
                rcMapping[channelReverseStateWanted] == chan) {

            // Check if the output is positive
            if (fval > 1750) {
                rcRev[rcMapping[channelReverseStateWanted]] = true;
            } else {
                rcRev[rcMapping[channelReverseStateWanted]] = false;
            }

            unsigned currRevFunc = channelReverseStateWanted;

            channelReverseStateWanted = -1;

            // Confirm found channel
            msgBox.setText(tr("%1 direction assigned").arg(channelNames[currRevFunc]));
            msgBox.setInformativeText(tr("%1").arg((rcRev[rcMapping[currRevFunc]]) ? tr("Reversed channel.") : tr("Did not reverse channel.") ));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            skipActionButton->hide();
            msgBox.removeButton(skipActionButton);
            (void)msgBox.exec();
        }
    }

    dataModelChanged = true;

    //qDebug() << "RC CHAN:" << chan << "PPM:" << fval << "NORMALIZED:" << normalized;
}

void QGCPX4VehicleConfig::updateAllInvertedCheckboxes()
{
    for (unsigned function_index = 0; function_index < chanMappedMax; function_index++) {

        int rc_input_index = rcMapping[function_index];

        if (rc_input_index < 0 || rc_input_index > (int)chanMax)
            continue;

        // Map index to checkbox.
        // TODO(lm) Would be better to stick the checkboxes into a vector upfront
        switch (function_index)
        {
        case 0:
            ui->invertCheckBox->setChecked(rcRev[rc_input_index]);
            ui->rollWidget->setName(tr("Roll (#%1)").arg(rcMapping[0] + 1));
            break;
        case 1:
            ui->invertCheckBox_2->setChecked(rcRev[rc_input_index]);
            ui->pitchWidget->setName(tr("Pitch (#%1)").arg(rcMapping[1] + 1));
            break;
        case 2:
            ui->invertCheckBox_3->setChecked(rcRev[rc_input_index]);
            ui->yawWidget->setName(tr("Yaw (#%1)").arg(rcMapping[2] + 1));
            break;
        case 3:
            ui->invertCheckBox_4->setChecked(rcRev[rc_input_index]);
            ui->throttleWidget->setName(tr("Throt. (#%1)").arg(rcMapping[3] + 1));
            break;
        case 4:
            ui->invertCheckBox_5->setChecked(rcRev[rc_input_index]);
            //ui->radio5Widget->setName(tr("Mode Switch (#%1)").arg(rcMapping[4] + 1));
            break;
        case 5:
            ui->assistSwInvertCheckBox->setChecked(rcRev[rc_input_index]);
            break;
        case 6:
            ui->missionSwInvertCheckBox->setChecked(rcRev[rc_input_index]);
            break;
        case 7:
            ui->returnSwInvertCheckBox->setChecked(rcRev[rc_input_index]);
            break;
        case 8:
            ui->flapsInvertCheckBox->setChecked(rcRev[rc_input_index]);
            break;
        case 9:
            ui->aux1InvertCheckBox->setChecked(rcRev[rc_input_index]);
            break;
        case 10:
            ui->aux2InvertCheckBox->setChecked(rcRev[rc_input_index]);
            break;
        }
    }
}

void QGCPX4VehicleConfig::updateMappingView(int function_index)
{
    Q_UNUSED(function_index);
    updateAllInvertedCheckboxes();

    QStringList assignments;

    for (unsigned i = 0; i < chanMax; i++) {
        assignments << "";
    }

    for (unsigned i = 0; i < chanMappedMax; i++) {
        if (rcMapping[i] >= 0 && rcMapping[i] < (int)chanMax) {
            assignments.replace(rcMapping[i], assignments[rcMapping[i]].append(QString(" / ").append(channelNames[i])));
        }
    }

    for (unsigned i = 0; i < chanMax; i++) {
        if (assignments[i] == "")
            assignments[i] = "UNUSED";
    }

    for (unsigned i = 0; i < chanMax; i++) {
        switch (i) {
        case 4:
            ui->radio5Widget->setName(tr("%1 (#5)").arg(assignments[4]));
            break;
        case 5:
            ui->radio6Widget->setName(tr("%1 (#6)").arg(assignments[5]));
            break;
        case 6:
            ui->radio7Widget->setName(tr("%1 (#7)").arg(assignments[6]));
            break;
        case 7:
            ui->radio8Widget->setName(tr("%1 (#8)").arg(assignments[7]));
            break;
        case 8:
            ui->radio9Widget->setName(tr("%1 (#9)").arg(assignments[8]));
            break;
        case 9:
            ui->radio10Widget->setName(tr("%1 (#10)").arg(assignments[9]));
            break;
        case 10:
            ui->radio11Widget->setName(tr("%1 (#11)").arg(assignments[10]));
            break;
        case 11:
            ui->radio12Widget->setName(tr("%1 (#12)").arg(assignments[11]));
            break;
        case 12:
            ui->radio13Widget->setName(tr("%1 (#13)").arg(assignments[12]));
            break;
        case 13:
            ui->radio14Widget->setName(tr("%1 (#14)").arg(assignments[13]));
            break;
        case 14:
            ui->radio15Widget->setName(tr("%1 (#15)").arg(assignments[14]));
            break;
        case 15:
            ui->radio16Widget->setName(tr("%1 (#16)").arg(assignments[15]));
            break;
        case 16:
            ui->radio17Widget->setName(tr("%1 (#17)").arg(assignments[16]));
            break;
        case 17:
            ui->radio18Widget->setName(tr("%1 (#18)").arg(assignments[17]));
            break;
        }
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
                setChannelToFunctionMapping(0, intValue);
                ui->rollSpinBox->setValue(rcMapping[0]+1);
                ui->rollSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_PITCH")) {
                setChannelToFunctionMapping(1, intValue);
                ui->pitchSpinBox->setValue(rcMapping[1]+1);
                ui->pitchSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_YAW")) {
                setChannelToFunctionMapping(2, intValue);
                ui->yawSpinBox->setValue(rcMapping[2]+1);
                ui->yawSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_THROTTLE")) {
                setChannelToFunctionMapping(3, intValue);
                ui->throttleSpinBox->setValue(rcMapping[3]+1);
                ui->throttleSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_MODE_SW")) {
                setChannelToFunctionMapping(4, intValue);
                ui->modeSpinBox->setValue(rcMapping[4]+1);
                ui->modeSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_ASSIST_SW")) {
                setChannelToFunctionMapping(5, intValue);
                ui->assistSwSpinBox->setValue(rcMapping[5]+1);
                ui->assistSwSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_MISSIO_SW")) {
                setChannelToFunctionMapping(6, intValue);
                ui->missionSwSpinBox->setValue(rcMapping[6]+1);
                ui->missionSwSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_RETURN_SW")) {
                setChannelToFunctionMapping(7, intValue);
                ui->returnSwSpinBox->setValue(rcMapping[7]+1);
                ui->returnSwSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_FLAPS")) {
                setChannelToFunctionMapping(8, intValue);
                ui->flapsSpinBox->setValue(rcMapping[8]+1);
                ui->flapsSpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_AUX1")) {
                setChannelToFunctionMapping(9, intValue);
                ui->aux1SpinBox->setValue(rcMapping[9]+1);
                ui->aux1SpinBox->setEnabled(true);
            }
            else if (parameterName.startsWith("RC_MAP_AUX2")) {
                setChannelToFunctionMapping(10, intValue);
                ui->aux2SpinBox->setValue(rcMapping[10]+1);
                ui->aux2SpinBox->setEnabled(true);
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
            // Not implemented at this point
//            else if (parameterName.startsWith("RC_SCALE_THROTTLE")) {
//                rcScaling[3] = floatVal;
//            }
//            else if (parameterName.startsWith("RC_SCALE_AUX1")) {
//                rcScaling[5] = floatVal;
//            }
//            else if (parameterName.startsWith("RC_SCALE_AUX2")) {
//                rcScaling[6] = floatVal;
//            }
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

                for (unsigned i = 0; i < chanMappedMax; i++)
                {
                    if (rcMapping[i] == (int)index)
                        updateMappingView(i);
                }
            }
        }
    }
}

void QGCPX4VehicleConfig::setChannelToFunctionMapping(int function, int channel)
{
    if (function >= 0 && function < (int)chanMappedMax)
        rcMapping[function] = channel;

    if (channel >= 0 && channel < (int)chanMax)
        rcToFunctionMapping[channel] = function;
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

void QGCPX4VehicleConfig::checktimeOuts()
{

}


void QGCPX4VehicleConfig::updateRcWidgetValues()
{
    ui->rollWidget->setValueAndRange(rcMappedValueRev[0],rcMappedMin[0],rcMappedMax[0]);
    ui->pitchWidget->setValueAndRange(rcMappedValueRev[1],rcMappedMin[1],rcMappedMax[1]);
    ui->yawWidget->setValueAndRange(rcMappedValueRev[2],rcMappedMin[2],rcMappedMax[2]);
    ui->throttleWidget->setValueAndRange(rcMappedValueRev[3],rcMappedMin[3],rcMappedMax[3]);

    ui->radio5Widget->setValueAndRange(rcValueReversed[4],rcMin[4],rcMax[4]);
    ui->radio6Widget->setValueAndRange(rcValueReversed[5],rcMin[5],rcMax[5]);
    ui->radio7Widget->setValueAndRange(rcValueReversed[6],rcMin[6],rcMax[6]);
    ui->radio8Widget->setValueAndRange(rcValueReversed[7],rcMin[7],rcMax[7]);
    ui->radio9Widget->setValueAndRange(rcValueReversed[8],rcMin[8],rcMax[8]);
    ui->radio10Widget->setValueAndRange(rcValueReversed[9],rcMin[9],rcMax[9]);
    ui->radio11Widget->setValueAndRange(rcValueReversed[10],rcMin[10],rcMax[10]);
    ui->radio12Widget->setValueAndRange(rcValueReversed[11],rcMin[11],rcMax[11]);
    ui->radio13Widget->setValueAndRange(rcValueReversed[12],rcMin[12],rcMax[12]);
    ui->radio14Widget->setValueAndRange(rcValueReversed[13],rcMin[13],rcMax[13]);
    ui->radio15Widget->setValueAndRange(rcValueReversed[14],rcMin[14],rcMax[14]);
    ui->radio16Widget->setValueAndRange(rcValueReversed[15],rcMin[15],rcMax[15]);
    ui->radio17Widget->setValueAndRange(rcValueReversed[16],rcMin[16],rcMax[16]);
    ui->radio18Widget->setValueAndRange(rcValueReversed[17],rcMin[17],rcMax[17]);
}

void QGCPX4VehicleConfig::updateRcChanLabels()
{
    ui->rollChanLabel->setText(labelForRcValue(rcRoll));
    ui->pitchChanLabel->setText(labelForRcValue(rcPitch));
    ui->yawChanLabel->setText(labelForRcValue(rcYaw));
    ui->throttleChanLabel->setText(labelForRcValue(rcThrottle));

    QString blankLabel = tr("---");
    if (rcValue[rcMapping[4]] != UINT16_MAX) {
        ui->modeChanLabel->setText(labelForRcValue(rcMode));
    }
    else {
        ui->modeChanLabel->setText(blankLabel);
    }

    if (rcValue[rcMapping[5]] != UINT16_MAX) {
        ui->assistSwChanLabel->setText(labelForRcValue(rcAssist));
    }
    else {
        ui->assistSwChanLabel->setText(blankLabel);
    }

    if (rcValue[rcMapping[6]] != UINT16_MAX) {
        ui->missionSwChanLabel->setText(labelForRcValue(rcMission));
    }
    else {
        ui->missionSwChanLabel->setText(blankLabel);
    }

    if (rcValue[rcMapping[7]] != UINT16_MAX) {
        ui->returnSwChanLabel->setText(labelForRcValue(rcReturn));
    }
    else {
        ui->returnSwChanLabel->setText(blankLabel);
    }

    if (rcValue[rcMapping[8]] != UINT16_MAX) {
        ui->flapsChanLabel->setText(labelForRcValue(rcFlaps));
    }
    else {
        ui->flapsChanLabel->setText(blankLabel);
    }

    if (rcValue[rcMapping[9]] != UINT16_MAX) {
        ui->aux1ChanLabel->setText(labelForRcValue(rcAux1));
    }
    else {
        ui->aux1ChanLabel->setText(blankLabel);
    }

    if (rcValue[rcMapping[10]] != UINT16_MAX) {
        ui->aux2ChanLabel->setText(labelForRcValue(rcAux2));
    }
    else {
        ui->aux2ChanLabel->setText(blankLabel);
    }
}

void QGCPX4VehicleConfig::updateView()
{
    if (dataModelChanged) {
        dataModelChanged = false;

        updateRcWidgetValues();
        updateRcChanLabels();
        if (chanCount > 0)
            ui->rcLabel->setText(tr("Radio control detected with %1 channels.").arg(chanCount));
    }

}

// On Windows (for VS2010) stdint.h contains the limits normally contained in limits.h
// It also needs the __STDC_LIMIT_MACROS macro defined in order to include them (done
// in qgroundcontrol.pri).
#ifdef WIN32
#include <stdint.h>
#else
#include <limits.h>
#endif

#include <QTimer>

#include "QGCVehicleConfig.h"
#include "UASManager.h"
#include "QGC.h"
#include "QGCToolWidget.h"
#include "ui_QGCVehicleConfig.h"

QGCVehicleConfig::QGCVehicleConfig(QWidget *parent) :
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
    changed(true),
    rc_mode(RC_MODE_2),
    calibrationEnabled(false),
    ui(new Ui::QGCVehicleConfig)
{
    setObjectName("QGC_VEHICLECONFIG");
    ui->setupUi(this);

    requestCalibrationRC();
    if (mav) mav->requestParameter(0, "RC_TYPE");

    ui->rcModeComboBox->setCurrentIndex((int)rc_mode - 1);

    ui->rcCalibrationButton->setCheckable(true);
    connect(ui->rcCalibrationButton, SIGNAL(clicked(bool)), this, SLOT(toggleCalibrationRC(bool)));
    connect(ui->storeButton, SIGNAL(clicked()), this, SLOT(writeParameters()));
    connect(ui->rcModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setRCModeIndex(int)));
    connect(ui->setTrimButton, SIGNAL(clicked()), this, SLOT(setTrimPositions()));

    /* Connect RC mapping assignments */
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

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    setActiveUAS(UASManager::instance()->getActiveUAS());

    for (unsigned int i = 0; i < chanMax; i++)
    {
        rcValue[i] = UINT16_MAX;
        rcMapping[i] = i;
    }

    updateTimer.setInterval(150);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateView()));
    updateTimer.start();
}

QGCVehicleConfig::~QGCVehicleConfig()
{
    delete ui;
}

void QGCVehicleConfig::setRCModeIndex(int newRcMode)
{
    if (newRcMode > 0 && newRcMode < 5)
    {
        rc_mode = (enum RC_MODE) (newRcMode+1);
        changed = true;
    }
}

void QGCVehicleConfig::toggleCalibrationRC(bool enabled)
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

void QGCVehicleConfig::setTrimPositions()
{
    // Set trim for roll, pitch, yaw, throttle
    rcTrim[rcMapping[0]] = rcValue[rcMapping[0]]; // roll
    rcTrim[rcMapping[1]] = rcValue[rcMapping[1]]; // pitch
    rcTrim[rcMapping[2]] = rcValue[rcMapping[2]]; // yaw
    // Set trim to min if stick is close to min
    if (abs(rcValue[rcMapping[3]] - rcMin[rcMapping[3]]) < 100)
    {
        rcTrim[rcMapping[3]] = rcMin[rcMapping[3]];   // throttle
    }
    // Set trim to max if stick is close to max
    if (abs(rcValue[rcMapping[3]] - rcMax[rcMapping[3]]) < 100)
    {
        rcTrim[rcMapping[3]] = rcMax[rcMapping[3]];   // throttle
    }
    rcTrim[rcMapping[4]] = ((rcMax[rcMapping[4]] - rcMin[rcMapping[4]]) / 2.0f) + rcMin[rcMapping[4]];   // mode sw
    rcTrim[rcMapping[5]] = ((rcMax[rcMapping[5]] - rcMin[rcMapping[5]]) / 2.0f) + rcMin[rcMapping[5]];   // aux 1
    rcTrim[rcMapping[6]] = ((rcMax[rcMapping[6]] - rcMin[rcMapping[6]]) / 2.0f) + rcMin[rcMapping[6]];   // aux 2
    rcTrim[rcMapping[7]] = ((rcMax[rcMapping[7]] - rcMin[rcMapping[7]]) / 2.0f) + rcMin[rcMapping[7]];   // aux 3
}

void QGCVehicleConfig::detectChannelInversion()
{

}

void QGCVehicleConfig::startCalibrationRC()
{
    ui->rcTypeComboBox->setEnabled(false);
    ui->rcCalibrationButton->setText(tr("Stop RC Calibration"));
    resetCalibrationRC();
    calibrationEnabled = true;
}

void QGCVehicleConfig::stopCalibrationRC()
{
    calibrationEnabled = false;
    ui->rcTypeComboBox->setEnabled(true);
    ui->rcCalibrationButton->setText(tr("Start RC Calibration"));
}

void QGCVehicleConfig::setActiveUAS(UASInterface* active)
{
    // Do nothing if system is the same or NULL
    if ((active == NULL) || mav == active) return;

    if (mav)
    {
        // Disconnect old system
        disconnect(mav, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
                   SLOT(remoteControlChannelRawChanged(int,float)));
        disconnect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
                   SLOT(parameterChanged(int,int,QString,QVariant)));

        foreach (QGCToolWidget* tool, toolWidgets)
        {
            delete tool;
        }
        toolWidgets.clear();
    }

    // Reset current state
    resetCalibrationRC();

    chanCount = 0;

    // Connect new system
    mav = active;
    connect(active, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
               SLOT(remoteControlChannelRawChanged(int,float)));
    connect(active, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
               SLOT(parameterChanged(int,int,QString,QVariant)));

    mav->requestParameters();

    QString defaultsDir = qApp->applicationDirPath() + "/files/" + mav->getAutopilotTypeName().toLower() + "/widgets/";

    QGCToolWidget* tool;

    // Load calibration
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_calibration.qgw", false))
    {
        toolWidgets.append(tool);
        ui->sensorLayout->addWidget(tool);
    } else {
        delete tool;
    }

    // Load multirotor attitude pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_mc_attitude_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->multiRotorAttitudeLayout->addWidget(tool);
    } else {
        delete tool;
    }

    // Load multirotor position pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_mc_position_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->multiRotorPositionLayout->addWidget(tool);
    } else {
        delete tool;
    }

    // Load fixed wing attitude pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_fw_attitude_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->fixedWingAttitudeLayout->addWidget(tool);
    } else {
        delete tool;
    }

    // Load fixed wing position pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_fw_position_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->fixedWingPositionLayout->addWidget(tool);
    } else {
        delete tool;
    }

    updateStatus(QString("Reading from system %1").arg(mav->getUASName()));
}

void QGCVehicleConfig::resetCalibrationRC()
{
    for (unsigned int i = 0; i < chanMax; ++i)
    {
        rcMin[i] = 1200;
        rcMax[i] = 1800;
    }
}

/**
 * Sends the RC calibration to the vehicle and stores it in EEPROM
 */
void QGCVehicleConfig::writeCalibrationRC()
{
    if (!mav) return;

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");

    // Do not write the RC type, as these values depend on this
    // active onboard parameter

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

void QGCVehicleConfig::requestCalibrationRC()
{
    if (!mav) return;

    QString minTpl("RC%1_MIN");
    QString maxTpl("RC%1_MAX");
    QString trimTpl("RC%1_TRIM");
    QString revTpl("RC%1_REV");

    // Do not request the RC type, as these values depend on this
    // active onboard parameter

    for (unsigned int i = 0; i < chanMax; ++i)
    {
        mav->requestParameter(0, minTpl.arg(i+1));
        QGC::SLEEP::usleep(5000);
        mav->requestParameter(0, trimTpl.arg(i+1));
        QGC::SLEEP::usleep(5000);
        mav->requestParameter(0, maxTpl.arg(i+1));
        QGC::SLEEP::usleep(5000);
        mav->requestParameter(0, revTpl.arg(i+1));
        QGC::SLEEP::usleep(5000);
    }
}

void QGCVehicleConfig::writeParameters()
{
    updateStatus(tr("Writing all onboard parameters."));
    writeCalibrationRC();
    mav->writeParametersToStorage();
}

void QGCVehicleConfig::remoteControlChannelRawChanged(int chan, float val)
{
    // Check if index and values are sane
    if (chan < 0 || static_cast<unsigned int>(chan) >= chanMax || val < 500 || val > 2500)
        return;

    if (chan + 1 > (int)chanCount) {
        chanCount = chan+1;
    }

    // Update calibration data
    if (calibrationEnabled) {
        if (val < rcMin[chan])
        {
            rcMin[chan] = val;
        }

        if (val > rcMax[chan])
        {
            rcMax[chan] = val;
        }
    }

    // Raw value
    rcValue[chan] = val;

    // Normalized value
    float normalized;

    if (val >= rcTrim[chan])
    {
        normalized = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
    }
    else
    {
        normalized = -(rcTrim[chan] - val)/(rcTrim[chan] - rcMin[chan]);
    }

    // Bound
    normalized = qBound(-1.0f, normalized, 1.0f);
    // Invert
    normalized = (rcRev[chan]) ? -1.0f*normalized : normalized;

    if (chan == rcMapping[0])
    {
        // ROLL
        rcRoll = normalized;
    }
    if (chan == rcMapping[1])
    {
        // PITCH
        rcPitch = normalized;
    }
    if (chan == rcMapping[2])
    {
        rcYaw = normalized;
    }
    if (chan == rcMapping[3])
    {
        // THROTTLE
        if (rcRev[chan]) {
            rcThrottle = 1.0f + normalized;
        } else {
            rcThrottle = normalized;
        }

        rcThrottle = qBound(0.0f, rcThrottle, 1.0f);
    }
    if (chan == rcMapping[4])
    {
        // MODE SWITCH
        rcMode = normalized;
    }
    if (chan == rcMapping[5])
    {
        // AUX1
        rcAux1 = normalized;
    }
    if (chan == rcMapping[6])
    {
        // AUX2
        rcAux2 = normalized;
    }
    if (chan == rcMapping[7])
    {
        // AUX3
        rcAux3 = normalized;
    }

    changed = true;

    //qDebug() << "RC CHAN:" << chan << "PPM:" << val << "NORMALIZED:" << normalized;
}

void QGCVehicleConfig::updateInvertedCheckboxes(int index)
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

void QGCVehicleConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);

    // Channel calibration values
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

    if (minTpl.exactMatch(parameterName)) {
        bool ok;
        int index = parameterName.mid(2, 1).toInt(&ok) - 1;
        //qDebug() << "PARAM:" << parameterName << "index:" << index;
        if (ok && (index >= 0) && (index < chanMax))
        {
            rcMin[index] = value.toInt();
        }
    }

    if (maxTpl.exactMatch(parameterName)) {
        bool ok;
        int index = parameterName.mid(2, 1).toInt(&ok) - 1;
        if (ok && (index >= 0) && (index < chanMax))
        {
            rcMax[index] = value.toInt();
        }
    }

    if (trimTpl.exactMatch(parameterName)) {
        bool ok;
        int index = parameterName.mid(2, 1).toInt(&ok) - 1;
        if (ok && (index >= 0) && (index < chanMax))
        {
            rcTrim[index] = value.toInt();
        }
    }

    if (revTpl.exactMatch(parameterName)) {
        bool ok;
        int index = parameterName.mid(2, 1).toInt(&ok) - 1;
        if (ok && (index >= 0) && (index < chanMax))
        {
            rcRev[index] = (value.toInt() == -1) ? true : false;
            updateInvertedCheckboxes(index);
        }
    }

//        mav->setParameter(0, trimTpl.arg(i), rcTrim[i]);
//        mav->setParameter(0, maxTpl.arg(i), rcMax[i]);
//        mav->setParameter(0, revTpl.arg(i), (rcRev[i]) ? -1 : 1);
//    }

    if (rcTypeUpdateRequested > 0 && parameterName == QString("RC_TYPE"))
    {
        rcTypeUpdateRequested = 0;
        updateStatus(tr("Received RC type update, setting parameters based on model."));
        rcType = value.toInt();
        // Request all other parameters as well
        requestCalibrationRC();
    }

    // Order is: roll, pitch, yaw, throttle, mode sw, aux 1-3

    if (parameterName.contains("RC_MAP_ROLL")) {
        rcMapping[0] = value.toInt() - 1;
        ui->rollSpinBox->setValue(rcMapping[0]+1);
    }

    if (parameterName.contains("RC_MAP_PITCH")) {
        rcMapping[1] = value.toInt() - 1;
        ui->pitchSpinBox->setValue(rcMapping[1]+1);
    }

    if (parameterName.contains("RC_MAP_YAW")) {
        rcMapping[2] = value.toInt() - 1;
        ui->yawSpinBox->setValue(rcMapping[2]+1);
    }

    if (parameterName.contains("RC_MAP_THROTTLE")) {
        rcMapping[3] = value.toInt() - 1;
        ui->throttleSpinBox->setValue(rcMapping[3]+1);
    }

    if (parameterName.contains("RC_MAP_MODE_SW")) {
        rcMapping[4] = value.toInt() - 1;
        ui->modeSpinBox->setValue(rcMapping[4]+1);
    }

    if (parameterName.contains("RC_MAP_AUX1")) {
        rcMapping[5] = value.toInt() - 1;
        ui->aux1SpinBox->setValue(rcMapping[5]+1);
    }

    if (parameterName.contains("RC_MAP_AUX2")) {
        rcMapping[6] = value.toInt() - 1;
        ui->aux2SpinBox->setValue(rcMapping[6]+1);
    }

    if (parameterName.contains("RC_MAP_AUX3")) {
        rcMapping[7] = value.toInt() - 1;
        ui->aux3SpinBox->setValue(rcMapping[7]+1);
    }

    // Scaling

    if (parameterName.contains("RC_SCALE_ROLL")) {
        rcScaling[0] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_PITCH")) {
        rcScaling[1] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_YAW")) {
        rcScaling[2] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_THROTTLE")) {
        rcScaling[3] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_MODE_SW")) {
        rcScaling[4] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_AUX1")) {
        rcScaling[5] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_AUX2")) {
        rcScaling[6] = value.toFloat();
    }

    if (parameterName.contains("RC_SCALE_AUX3")) {
        rcScaling[7] = value.toFloat();
    }
}

void QGCVehicleConfig::updateStatus(const QString& str)
{
    ui->statusLabel->setText(str);
    ui->statusLabel->setStyleSheet("");
}

void QGCVehicleConfig::updateError(const QString& str)
{
    ui->statusLabel->setText(str);
    ui->statusLabel->setStyleSheet(QString("QLabel { margin: 0px 2px; font: 14px; color: %1; background-color: %2; }").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.name()));
}

void QGCVehicleConfig::setRCType(int type)
{
    if (!mav) return;

    // XXX TODO Add handling of RC_TYPE vs non-RC_TYPE here

    mav->setParameter(0, "RC_TYPE", type);
    rcTypeUpdateRequested = QGC::groundTimeMilliseconds();
    QTimer::singleShot(rcTypeTimeout+100, this, SLOT(checktimeOuts()));
}

void QGCVehicleConfig::checktimeOuts()
{
    if (rcTypeUpdateRequested > 0)
    {
        if (QGC::groundTimeMilliseconds() - rcTypeUpdateRequested > rcTypeTimeout)
        {
            updateError(tr("Setting remote control timed out - is the system connected?"));
        }
    }
}

void QGCVehicleConfig::updateView()
{
    if (changed)
    {
        if (rc_mode == RC_MODE_1)
        {
            ui->rollSlider->setValue(rcRoll * 50 + 50);
            ui->pitchSlider->setValue(rcThrottle * 100);
            ui->yawSlider->setValue(rcYaw * 50 + 50);
            ui->throttleSlider->setValue(rcPitch * 50 + 50);
        }
        else if (rc_mode == RC_MODE_2)
        {
            ui->rollSlider->setValue(rcRoll * 50 + 50);
            ui->pitchSlider->setValue(rcPitch * 50 + 50);
            ui->yawSlider->setValue(rcYaw * 50 + 50);
            ui->throttleSlider->setValue(rcThrottle * 100);
        }
        else if (rc_mode == RC_MODE_3)
        {
            ui->rollSlider->setValue(rcYaw * 50 + 50);
            ui->pitchSlider->setValue(rcThrottle * 100);
            ui->yawSlider->setValue(rcRoll * 50 + 50);
            ui->throttleSlider->setValue(rcPitch * 50 + 50);
        }
        else if (rc_mode == RC_MODE_4)
        {
            ui->rollSlider->setValue(rcYaw * 50 + 50);
            ui->pitchSlider->setValue(rcPitch * 50 + 50);
            ui->yawSlider->setValue(rcRoll * 50 + 50);
            ui->throttleSlider->setValue(rcThrottle * 100);
        }

        ui->chanLabel->setText(QString("%1/%2").arg(rcValue[rcMapping[0]]).arg(rcRoll, 5, 'f', 2, QChar(' ')));
        ui->chanLabel_2->setText(QString("%1/%2").arg(rcValue[rcMapping[1]]).arg(rcPitch, 5, 'f', 2, QChar(' ')));
        ui->chanLabel_3->setText(QString("%1/%2").arg(rcValue[rcMapping[2]]).arg(rcYaw, 5, 'f', 2, QChar(' ')));
        ui->chanLabel_4->setText(QString("%1/%2").arg(rcValue[rcMapping[3]]).arg(rcThrottle, 5, 'f', 2, QChar(' ')));

        ui->modeSwitchSlider->setValue(rcMode * 50 + 50);
        ui->chanLabel_5->setText(QString("%1/%2").arg(rcValue[rcMapping[4]]).arg(rcMode, 5, 'f', 2, QChar(' ')));

        if (rcValue[rcMapping[5]] != UINT16_MAX) {
            ui->aux1Slider->setValue(rcAux1 * 50 + 50);
            ui->chanLabel_6->setText(QString("%1/%2").arg(rcValue[rcMapping[5]]).arg(rcAux1, 5, 'f', 2, QChar(' ')));
        } else {
            ui->chanLabel_6->setText(tr("---"));
        }

        if (rcValue[rcMapping[6]] != UINT16_MAX) {
            ui->aux2Slider->setValue(rcAux2 * 50 + 50);
            ui->chanLabel_7->setText(QString("%1/%2").arg(rcValue[rcMapping[6]]).arg(rcAux2, 5, 'f', 2, QChar(' ')));
        } else {
            ui->chanLabel_7->setText(tr("---"));
        }

        if (rcValue[rcMapping[7]] != UINT16_MAX) {
            ui->aux3Slider->setValue(rcAux3 * 50 + 50);
            ui->chanLabel_8->setText(QString("%1/%2").arg(rcValue[rcMapping[7]]).arg(rcAux3, 5, 'f', 2, QChar(' ')));
        } else {
            ui->chanLabel_8->setText(tr("---"));
        }

        changed = false;
    }
}

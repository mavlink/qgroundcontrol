#include <limits.h>

#include <QTimer>

#include "QGCVehicleConfig.h"
#include "UASManager.h"
#include "QGC.h"
#include "QGCToolWidget.h"
#include "ui_QGCVehicleConfig.h"

QGCVehicleConfig::QGCVehicleConfig(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    changed(true),
    rc_mode(RC_MODE_2),
    rcRoll(0.0f),
    rcPitch(0.0f),
    rcYaw(0.0f),
    rcThrottle(0.0f),
    rcMode(0.0f),
    rcAux1(0.0f),
    rcAux2(0.0f),
    rcAux3(0.0f),
    ui(new Ui::QGCVehicleConfig)
{
    setObjectName("QGC_VEHICLECONFIG");
    ui->setupUi(this);

    requestCalibrationRC();
    if (mav) mav->requestParameter(0, "RC_TYPE");

    ui->rcModeComboBox->setCurrentIndex((int)rc_mode - 1);

    connect(ui->rcCalibrationButton, SIGNAL(clicked(bool)), this, SLOT(toggleCalibrationRC(bool)));
    connect(ui->storeButton, SIGNAL(clicked()), this, SLOT(writeParameters()));
    connect(ui->rcModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setRCModeIndex(int)));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    setActiveUAS(UASManager::instance()->getActiveUAS());

    for (unsigned int i = 0; i < chanMax; i++)
    {
        rcValue[i] = 1500;
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

void QGCVehicleConfig::startCalibrationRC()
{
    ui->rcTypeComboBox->setEnabled(false);
    ui->rcCalibrationButton->setText(tr("Stop RC Calibration"));
    resetCalibrationRC();
}

void QGCVehicleConfig::stopCalibrationRC()
{
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
    }

    // Reset current state
    resetCalibrationRC();

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
    }
    // Load multirotor attitude pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_mc_attitude_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->multiRotorAttitudeLayout->addWidget(tool);
    }

    // Load multirotor position pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_mc_position_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->multiRotorPositionLayout->addWidget(tool);
    }

    // Load fixed wing attitude pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_fw_attitude_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->fixedWingAttitudeLayout->addWidget(tool);
    }

    // Load fixed wing position pid
    tool = new QGCToolWidget("", this);
    if (tool->loadSettings(defaultsDir + "px4_fw_position_pid_params.qgw", false))
    {
        toolWidgets.append(tool);
        ui->fixedWingPositionLayout->addWidget(tool);
    }

    updateStatus(QString("Reading from system %1").arg(mav->getUASName()));
}

void QGCVehicleConfig::resetCalibrationRC()
{
    for (unsigned int i = 0; i < chanMax; ++i)
    {
        rcMin[i] = INT_MAX;
        rcMax[i] = INT_MIN;
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

    for (unsigned int i = 0; i < chanMax; ++i)
    {
        mav->setParameter(0, minTpl.arg(i), rcMin[i]);
        mav->setParameter(0, trimTpl.arg(i), rcTrim[i]);
        mav->setParameter(0, maxTpl.arg(i), rcMax[i]);
        mav->setParameter(0, revTpl.arg(i), (rcRev[i]) ? -1 : 1);
    }

    // Write mappings
    mav->setParameter(0, "RC_MAP_ROLL", rcMapping[0]);
    mav->setParameter(0, "RC_MAP_PITCH", rcMapping[1]);
    mav->setParameter(0, "RC_MAP_THROTTLE", rcMapping[2]);
    mav->setParameter(0, "RC_MAP_YAW", rcMapping[3]);
    mav->setParameter(0, "RC_MAP_MODE_SW", rcMapping[4]);
    mav->setParameter(0, "RC_MAP_AUX1", rcMapping[5]);
    mav->setParameter(0, "RC_MAP_AUX2", rcMapping[6]);
    mav->setParameter(0, "RC_MAP_AUX3", rcMapping[7]);
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
        mav->requestParameter(0, minTpl.arg(i));
        QGC::SLEEP::usleep(5000);
        mav->requestParameter(0, trimTpl.arg(i));
        QGC::SLEEP::usleep(5000);
        mav->requestParameter(0, maxTpl.arg(i));
        QGC::SLEEP::usleep(5000);
        mav->requestParameter(0, revTpl.arg(i));
        QGC::SLEEP::usleep(5000);
    }
}

void QGCVehicleConfig::writeParameters()
{
    updateStatus(tr("Writing all onboard parameters."));
    writeCalibrationRC();
}

void QGCVehicleConfig::remoteControlChannelRawChanged(int chan, float val)
{
//    /* scale around the mid point differently for lower and upper range */
//            if (ppm_buffer[i] > _rc.chan[i].mid + _parameters.dz[i]) {
//                _rc.chan[i].scaled = ((ppm_buffer[i] - _parameters.trim[i]) / (_parameters.max[i] - _parameters.trim[i]));
//            } else if ((ppm_buffer[i] < _rc_chan[i].mid - _parameters.dz[i])) {
//                _rc.chan[i].scaled = -1.0 + ((ppm_buffer[i] - _parameters.min[i]) / (_parameters.trim[i] - _parameters.min[i]));
//            } else {
//                /* in the configured dead zone, output zero */
//                _rc.chan[i].scaled = 0.0f;
//            }

    if (chan < 0 || static_cast<unsigned int>(chan) >= chanMax)
        return;

    if (val < rcMin[chan])
    {
        rcMin[chan] = val;
    }

    if (val > rcMax[chan])
    {
        rcMax[chan] = val;
    }

    if (chan == rcMapping[0])
    {
        // ROLL
        if (rcRoll >= rcTrim[chan])
        {
            rcRoll = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
        }
        else
        {
            rcRoll = (val - rcMin[chan])/(rcTrim[chan] - rcMin[chan]);
        }

        rcValue[0] = val;
        rcRoll = qBound(-1.0f, rcRoll, 1.0f);
    }
    else if (chan == rcMapping[1])
    {
        // PITCH
        if (rcPitch >= rcTrim[chan])
        {
            rcPitch = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
        }
        else
        {
            rcPitch = (val - rcMin[chan])/(rcTrim[chan] - rcMin[chan]);
        }
        rcValue[1] = val;
        rcPitch = qBound(-1.0f, rcPitch, 1.0f);
    }
    else if (chan == rcMapping[2])
    {
        // YAW
        if (rcYaw >= rcTrim[chan])
        {
            rcYaw = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
        }
        else
        {
            rcYaw = (val - rcMin[chan])/(rcTrim[chan] - rcMin[chan]);
        }
        rcValue[2] = val;
        rcYaw = qBound(-1.0f, rcYaw, 1.0f);
    }
    else if (chan == rcMapping[3])
    {
        // THROTTLE
        if (rcThrottle >= rcTrim[chan])
        {
            rcThrottle = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
        }
        else
        {
            rcThrottle = (val - rcMin[chan])/(rcTrim[chan] - rcMin[chan]);
        }
        rcValue[3] = val;
        rcThrottle = qBound(-1.0f, rcThrottle, 1.0f);
    }
    else if (chan == rcMapping[4])
    {
        // MODE SWITCH
        if (rcMode >= rcTrim[chan])
        {
            rcMode = (val - rcTrim[chan])/(rcMax[chan] - rcTrim[chan]);
        }
        else
        {
            rcMode = (val - rcMin[chan])/(rcTrim[chan] - rcMin[chan]);
        }
        rcValue[4] = val;
        rcMode = qBound(-1.0f, rcMode, 1.0f);
    }
    else if (chan == rcMapping[5])
    {
        // AUX1
        rcAux1 = val;
        rcValue[5] = val;
    }
    else if (chan == rcMapping[6])
    {
        // AUX2
        rcAux2 = val;
        rcValue[6] = val;
    }
    else if (chan == rcMapping[7])
    {
        // AUX3
        rcAux3 = val;
        rcValue[7] = val;
    }

    changed = true;

    //qDebug() << "RC CHAN:" << chan << "PPM:" << val;
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
        unsigned int index = parameterName.mid(2, 1).toInt(&ok);
        if (ok && (index > 0) && (index < chanMax))
        {
            rcMin[index] = value.toInt();
        }
    }

    if (maxTpl.exactMatch(parameterName)) {
        bool ok;
        unsigned int index = parameterName.mid(2, 1).toInt(&ok);
        if (ok && (index > 0) && (index < chanMax))
        {
            rcMax[index] = value.toInt();
        }
    }

    if (trimTpl.exactMatch(parameterName)) {
        bool ok;
        unsigned int index = parameterName.mid(2, 1).toInt(&ok);
        if (ok && (index > 0) && (index < chanMax))
        {
            rcTrim[index] = value.toInt();
        }
    }

    if (revTpl.exactMatch(parameterName)) {
        bool ok;
        unsigned int index = parameterName.mid(2, 1).toInt(&ok);
        if (ok && (index > 0) && (index < chanMax))
        {
            rcRev[index] = (value.toInt() == -1) ? true : false;

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
        rcMapping[0] = value.toInt();
        ui->rollSpinBox->setValue(rcMapping[0]);
    }

    if (parameterName.contains("RC_MAP_PITCH")) {
        rcMapping[1] = value.toInt();
        ui->pitchSpinBox->setValue(rcMapping[1]);
    }

    if (parameterName.contains("RC_MAP_YAW")) {
        rcMapping[2] = value.toInt();
        ui->yawSpinBox->setValue(rcMapping[2]);
    }

    if (parameterName.contains("RC_MAP_THROTTLE")) {
        rcMapping[3] = value.toInt();
        ui->throttleSpinBox->setValue(rcMapping[3]);
    }

    if (parameterName.contains("RC_MAP_MODE_SW")) {
        rcMapping[4] = value.toInt();
        ui->modeSpinBox->setValue(rcMapping[4]);
    }

    if (parameterName.contains("RC_MAP_AUX1")) {
        rcMapping[5] = value.toInt();
        ui->aux1SpinBox->setValue(rcMapping[5]);
    }

    if (parameterName.contains("RC_MAP_AUX2")) {
        rcMapping[6] = value.toInt();
        ui->aux1SpinBox->setValue(rcMapping[6]);
    }

    if (parameterName.contains("RC_MAP_AUX3")) {
        rcMapping[7] = value.toInt();
        ui->aux1SpinBox->setValue(rcMapping[7]);
    }

    // Scaling

    if (parameterName.contains("RC_SCALE_ROLL")) {
        rcScaling[0] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_PITCH")) {
        rcScaling[1] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_YAW")) {
        rcScaling[2] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_THROTTLE")) {
        rcScaling[3] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_MODE_SW")) {
        rcScaling[4] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_AUX1")) {
        rcScaling[5] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_AUX2")) {
        rcScaling[6] = value.toInt();
    }

    if (parameterName.contains("RC_SCALE_AUX3")) {
        rcScaling[7] = value.toInt();
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
            ui->rollSlider->setValue(rcRoll);
            ui->pitchSlider->setValue(rcThrottle);
            ui->yawSlider->setValue(rcYaw);
            ui->throttleSlider->setValue(rcPitch);
        }
        else if (rc_mode == RC_MODE_2)
        {
            ui->rollSlider->setValue(rcRoll);
            ui->pitchSlider->setValue(rcPitch);
            ui->yawSlider->setValue(rcYaw);
            ui->throttleSlider->setValue(rcThrottle);
        }
        else if (rc_mode == RC_MODE_3)
        {
            ui->rollSlider->setValue(rcYaw);
            ui->pitchSlider->setValue(rcThrottle);
            ui->yawSlider->setValue(rcRoll);
            ui->throttleSlider->setValue(rcPitch);
        }
        else if (rc_mode == RC_MODE_4)
        {
            ui->rollSlider->setValue(rcYaw);
            ui->pitchSlider->setValue(rcPitch);
            ui->yawSlider->setValue(rcRoll);
            ui->throttleSlider->setValue(rcThrottle);
        }

        ui->chanLabel->setText(QString("%1/%2").arg(rcValue[0]).arg(rcRoll));
        ui->chanLabel_2->setText(QString("%1/%2").arg(rcValue[1]).arg(rcPitch));
        ui->chanLabel_3->setText(QString("%1/%2").arg(rcValue[2]).arg(rcYaw));
        ui->chanLabel_4->setText(QString("%1/%2").arg(rcValue[3]).arg(rcThrottle));
        ui->modeSwitchSlider->setValue(rcMode);
        ui->chanLabel_5->setText(QString("%1/%2").arg(rcValue[4]).arg(rcMode));
        ui->aux1Slider->setValue(rcAux1);
        ui->chanLabel_6->setText(QString("%1/%2").arg(rcValue[5]).arg(rcAux1));
        ui->aux2Slider->setValue(rcAux2);
        ui->chanLabel_7->setText(QString("%1/%2").arg(rcValue[6]).arg(rcAux2));
        ui->aux3Slider->setValue(rcAux3);
        ui->chanLabel_8->setText(QString("%1/%2").arg(rcValue[7]).arg(rcAux3));
        changed = false;
    }
}

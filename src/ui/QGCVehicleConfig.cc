#include <limits.h>

#include <QTimer>

#include "QGCVehicleConfig.h"
#include "UASManager.h"
#include "QGC.h"
#include "ui_QGCVehicleConfig.h"

QGCVehicleConfig::QGCVehicleConfig(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    changed(true),
    ui(new Ui::QGCVehicleConfig)
{
    setObjectName("QGC_VEHICLECONFIG");
    ui->setupUi(this);

    requestCalibrationRC();
    if (mav) mav->requestParameter(0, "RC_TYPE");

    connect(ui->rcCalibrationButton, SIGNAL(clicked(bool)), this, SLOT(toggleCalibrationRC(bool)));
    connect(ui->storeButton, SIGNAL(clicked()), this, SLOT(writeParameters()));

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
    resetCalibrationRC();
}

void QGCVehicleConfig::stopCalibrationRC()
{
    ui->rcTypeComboBox->setEnabled(true);
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
        resetCalibrationRC();
    }

    // Connect new system
    mav = active;
    connect(active, SIGNAL(remoteControlChannelRawChanged(int,float)), this,
               SLOT(remoteControlChannelRawChanged(int,float)));
    connect(active, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,
               SLOT(parameterChanged(int,int,QString,QVariant)));
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
    }
    else if (chan == rcMapping[6])
    {
        // AUX2
        rcAux2 = val;
    }
    else if (chan == rcMapping[7])
    {
        // AUX3
        rcAux3 = val;
    }

    changed = true;

    //qDebug() << "RC CHAN:" << chan << "PPM:" << val;
}

void QGCVehicleConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    if (rcTypeUpdateRequested > 0 && parameterName == QString("RC_TYPE"))
    {
        rcTypeUpdateRequested = 0;
        updateStatus(tr("Received RC type update, setting parameters based on model."));
        rcType = value.toInt();
        // Request all other parameters as well
        requestCalibrationRC();
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
        ui->rollSlider->setValue(rcValue[0]);
        ui->pitchSlider->setValue(rcValue[1]);
        ui->yawSlider->setValue(rcValue[2]);
        ui->throttleSlider->setValue(rcValue[3]);
        changed = false;
    }
}

/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2013 Michael Carpenter (malcom2073@gmail.com)

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

/**
 * @file
 *   @brief Radio Calibration Configuration source.
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 *
 */

#include "RadioCalibrationConfig.h"
#include <QMessageBox>

RadioCalibrationConfig::RadioCalibrationConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);

    connect(ui.calibrateButton,SIGNAL(clicked()),this,SLOT(calibrateButtonClicked()));
    m_calibrationEnabled = false;
    ui.rollWidget->setMin(800);
    ui.rollWidget->setMax(2200);
    ui.pitchWidget->setMin(800);
    ui.pitchWidget->setMax(2200);
    ui.throttleWidget->setMin(800);
    ui.throttleWidget->setMax(2200);
    ui.yawWidget->setMin(800);
    ui.yawWidget->setMax(2200);
    ui.radio5Widget->setMin(800);
    ui.radio5Widget->setMax(2200);
    ui.radio6Widget->setMin(800);
    ui.radio6Widget->setMax(2200);
    ui.radio7Widget->setMin(800);
    ui.radio7Widget->setMax(2200);
    ui.radio8Widget->setMin(800);
    ui.radio8Widget->setMax(2200);
    ui.rollWidget->setOrientation(Qt::Horizontal);
    ui.rollWidget->setName("Roll");
    ui.yawWidget->setOrientation(Qt::Horizontal);
    ui.yawWidget->setName("Yaw");
    ui.pitchWidget->setName("Pitch");
    ui.throttleWidget->setName("Throttle");
    ui.radio5Widget->setOrientation(Qt::Horizontal);
    ui.radio5Widget->setName("Radio 5");
    ui.radio6Widget->setOrientation(Qt::Horizontal);
    ui.radio6Widget->setName("Radio 6");
    ui.radio7Widget->setOrientation(Qt::Horizontal);
    ui.radio7Widget->setName("Radio 7");
    ui.radio8Widget->setOrientation(Qt::Horizontal);
    ui.radio8Widget->setName("Radio 8");

    guiUpdateTimer = new QTimer(this);
    connect(guiUpdateTimer,SIGNAL(timeout()),this,SLOT(guiUpdateTimerTick()));

    rcMin << 1100.0 << 1100.0 << 1100.0 << 1100.0 << 1100.0 << 1100.0 << 1100.0 << 1100.0;
    rcMax << 1900.0 << 1900.0 << 1900.0 << 1900.0 << 1900.0 << 1900.0 << 1900.0 << 1900.0;
    rcTrim << 1500.0 << 1500.0 << 1500.0 << 1500.0 << 1500.0 << 1500.0 << 1500.0 << 1500.0;
    rcValue << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
}

RadioCalibrationConfig::~RadioCalibrationConfig()
{
}
void RadioCalibrationConfig::activeUASSet(UASInterface *uas)
{
    if (m_uas)
    {
        disconnect(m_uas, SIGNAL(remoteControlChannelRawChanged(int,float)), this,SLOT(remoteControlChannelRawChanged(int,float)));
    }
    AP2ConfigWidget::activeUASSet(uas);
    if (!uas)
    {
        return;
    }
    connect(m_uas,SIGNAL(remoteControlChannelRawChanged(int,float)),this,SLOT(remoteControlChannelRawChanged(int,float)));
}
void RadioCalibrationConfig::remoteControlChannelRawChanged(int chan,float val)
{

    //Channel is 0-7 typically?
    //Val will be 0-3000, PWM value.
    if (m_calibrationEnabled) {
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
}

void RadioCalibrationConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{

}
void RadioCalibrationConfig::guiUpdateTimerTick()
{
    ui.rollWidget->setValue(rcValue[0]);
    ui.pitchWidget->setValue(rcValue[1]);
    ui.throttleWidget->setValue(rcValue[2]);
    ui.yawWidget->setValue(rcValue[3]);
    ui.radio5Widget->setValue(rcValue[4]);
    ui.radio6Widget->setValue(rcValue[5]);
    ui.radio7Widget->setValue(rcValue[6]);
    ui.radio8Widget->setValue(rcValue[7]);
    if (m_calibrationEnabled)
    {
        ui.rollWidget->setMin(rcMin[0]);
        ui.rollWidget->setMax(rcMax[0]);
        ui.pitchWidget->setMin(rcMin[1]);
        ui.pitchWidget->setMax(rcMax[1]);
        ui.throttleWidget->setMin(rcMin[2]);
        ui.throttleWidget->setMax(rcMax[2]);
        ui.yawWidget->setMin(rcMin[3]);
        ui.yawWidget->setMax(rcMax[3]);
        ui.radio5Widget->setMin(rcMin[4]);
        ui.radio5Widget->setMax(rcMax[4]);
        ui.radio6Widget->setMin(rcMin[5]);
        ui.radio6Widget->setMax(rcMax[5]);
        ui.radio7Widget->setMin(rcMin[6]);
        ui.radio7Widget->setMax(rcMax[6]);
        ui.radio8Widget->setMin(rcMin[7]);
        ui.radio8Widget->setMax(rcMax[7]);
    }
}
void RadioCalibrationConfig::showEvent(QShowEvent *event)
{
    guiUpdateTimer->start(100);
}
void RadioCalibrationConfig::hideEvent(QHideEvent *event)
{
    guiUpdateTimer->stop();
}
void RadioCalibrationConfig::calibrateButtonClicked()
{
    if (!m_calibrationEnabled)
    {
        ui.calibrateButton->setText("End Calibration");
        QMessageBox::information(0,"Warning!","You are about to start radio calibration.\nPlease ensure all motor power is disconnected AND all props are removed from the vehicle.\nAlso ensure transmitter and reciever are powered and connected\n\nClick OK to confirm");
        m_calibrationEnabled = true;
        for (int i=0;i<8;i++)
        {
            rcMin[i] = 1500;
            rcMax[i] = 1500;
        }
        ui.rollWidget->showMinMax();
        ui.pitchWidget->showMinMax();
        ui.yawWidget->showMinMax();
        ui.radio5Widget->showMinMax();
        ui.radio6Widget->showMinMax();
        ui.radio7Widget->showMinMax();
        ui.throttleWidget->showMinMax();
        ui.radio8Widget->showMinMax();
        QMessageBox::information(0,"Information","Click OK, then move all sticks to their extreme positions, watching the min/max values to ensure you get the most range from your controller. This includes all switches");
    }
    else
    {
        ui.calibrateButton->setText("Calibrate");
        QMessageBox::information(0,"Trims","Ensure all sticks are centered and throttle is in the downmost position, click OK to continue");
        ///TODO: Set trims!
        m_calibrationEnabled = false;
        ui.rollWidget->hideMinMax();
        ui.pitchWidget->hideMinMax();
        ui.yawWidget->hideMinMax();
        ui.radio5Widget->hideMinMax();
        ui.radio6Widget->hideMinMax();
        ui.throttleWidget->hideMinMax();
        ui.radio7Widget->hideMinMax();
        ui.radio8Widget->hideMinMax();
        QString statusstr;
        statusstr = "Below you will find the detected radio calibration information that will be sent to the autopilot\n";
        statusstr += "Normal values are around 1100 to 1900, with disconnected channels reading very close to 1500\n\n";
        statusstr += "Channel\tMin\tCenter\tMax\n";
        statusstr += "--------------------\n";
        for (int i=0;i<8;i++)
        {
            statusstr += QString::number(i) + "\t" + QString::number(rcMin[i]) + "\t" + QString::number(rcValue[i]) + "\t" + QString::number(rcMax[i]) + "\n";
        }
        QMessageBox::information(0,"Status",statusstr);
        //Send calibrations.
        QString minTpl("RC%1_MIN");
        QString maxTpl("RC%1_MAX");
        //QString trimTpl("RC%1_TRIM");

        // Do not write the RC type, as these values depend on this
        // active onboard parameter

        for (unsigned int i = 0; i < 8; ++i)
        {
            qDebug() << "SENDING MIN" << minTpl.arg(i+1) << rcMin[i];
            qDebug() << "SENDING MAX" << maxTpl.arg(i+1) << rcMax[i];
            m_uas->getParamManager()->setParameter(1, minTpl.arg(i+1), (float)rcMin[i]);
            QGC::SLEEP::usleep(50000);
            //m_uas->setParameter(0, trimTpl.arg(i+1), rcTrim[i]);
            //QGC::SLEEP::usleep(50000);
            m_uas->getParamManager()->setParameter(1, maxTpl.arg(i+1), (float)rcMax[i]);
            QGC::SLEEP::usleep(50000);
        }
        ui.rollWidget->setMin(800);
        ui.rollWidget->setMax(2200);
        ui.pitchWidget->setMin(800);
        ui.pitchWidget->setMax(2200);
        ui.throttleWidget->setMin(800);
        ui.throttleWidget->setMax(2200);
        ui.yawWidget->setMin(800);
        ui.yawWidget->setMax(2200);
        ui.radio5Widget->setMin(800);
        ui.radio5Widget->setMax(2200);
        ui.radio6Widget->setMin(800);
        ui.radio6Widget->setMax(2200);
        ui.radio7Widget->setMin(800);
        ui.radio7Widget->setMax(2200);
        ui.radio8Widget->setMin(800);
        ui.radio8Widget->setMax(2200);

    }
}

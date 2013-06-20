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


RadioCalibrationConfig::RadioCalibrationConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    m_uas = 0;
    m_calibrationEnabled = false;
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    setActiveUAS(UASManager::instance()->getActiveUAS());
    ui.rollWidget->setMin(800);
    ui.rollWidget->setMax(2200);
    ui.pitchWidget->setMin(800);
    ui.pitchWidget->setMax(2200);
    ui.throttleWidget->setMin(800);
    ui.throttleWidget->setMax(2200);
    ui.yawWidget->setMin(800);
    ui.yawWidget->setMax(2200);
    guiUpdateTimer = new QTimer(this);
    connect(guiUpdateTimer,SIGNAL(timeout()),this,SLOT(guiUpdateTimerTick()));
}

RadioCalibrationConfig::~RadioCalibrationConfig()
{
}
void RadioCalibrationConfig::setActiveUAS(UASInterface *uas)
{
    if (uas==NULL) return;
    if (m_uas)
    {
        // Disconnect old system
        disconnect(m_uas, SIGNAL(remoteControlChannelRawChanged(int,float)), this,SLOT(remoteControlChannelRawChanged(int,float)));
        disconnect(m_uas, SIGNAL(parameterChanged(int,int,QString,QVariant)), this,SLOT(parameterChanged(int,int,QString,QVariant)));
    }

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
}
void RadioCalibrationConfig::showEvent(QShowEvent *event)
{
    guiUpdateTimer->stop();
}
void RadioCalibrationConfig::hideEvent(QHideEvent *event)
{
    guiUpdateTimer->start(100);
}

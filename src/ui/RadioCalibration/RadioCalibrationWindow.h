/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Main window for radio calibration
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef RADIOCALIBRATIONWINDOW_H
#define RADIOCALIBRATIONWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QPointer>
#include <QFileDialog>
#include <QFile>
#include <QtXml>
#include <QTextStream>

#include <stdint.h>

#include "AirfoilServoCalibrator.h"
#include "SwitchCalibrator.h"
#include "CurveCalibrator.h"

#include "mavlink.h"
#include "mavlink_types.h"
#include "UAS.h"
#include "UASManager.h"
#include "RadioCalibrationData.h"

/**
  @brief Main window for radio calibration
  @author Bryan Godbolt <godbolt@ece.ualberta.ca>
  */
class RadioCalibrationWindow : public QWidget
{
    Q_OBJECT

public:
    explicit RadioCalibrationWindow(QWidget *parent = 0);

public slots:
    void setChannel(int ch, float raw);
    void loadFile();
    void saveFile();
    void send();
    void request();
    void receive(const QPointer<RadioCalibrationData>& radio);
    void setUASId(int id) {
        this->uasId = id;
    }


protected:
    AirfoilServoCalibrator *aileron;
    AirfoilServoCalibrator *elevator;
    AirfoilServoCalibrator *rudder;
    SwitchCalibrator *gyro;
    CurveCalibrator *pitch;
    CurveCalibrator *throttle;
    int uasId;
    QPointer<RadioCalibrationData> radio;
    QSignalMapper mapper;

    void parseSetpoint(const QDomElement& setpoint, const QPointer<RadioCalibrationData>& radio);
};

#endif // RADIOCALIBRATIONWINDOW_H

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
 *   @brief Radio Calibration Configuration widget header.
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 *
 */

#ifndef RADIOCALIBRATIONCONFIG_H
#define RADIOCALIBRATIONCONFIG_H

#include <QWidget>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include "ui_RadioCalibrationConfig.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "AP2ConfigWidget.h"
class RadioCalibrationConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit RadioCalibrationConfig(QWidget *parent = 0);
    ~RadioCalibrationConfig();
protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
private slots:
    void activeUASSet(UASInterface *uas);
    void remoteControlChannelRawChanged(int chan,float val);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void guiUpdateTimerTick();
    void calibrateButtonClicked();
private:
    double rcMin[8];
    double rcMax[8];
    double rcTrim[8];
    double rcValue[8];
    QTimer *guiUpdateTimer;
    bool m_calibrationEnabled;
    Ui::RadioCalibrationConfig ui;
};

#endif // RADIOCALIBRATIONCONFIG_H

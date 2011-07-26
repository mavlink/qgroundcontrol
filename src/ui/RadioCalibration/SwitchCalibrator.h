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
 *   @brief Calibration widget for 2 setpoint switch
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef SWITCHCALIBRATOR_H
#define SWITCHCALIBRATOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>

#include "AbstractCalibrator.h"

/**
  @brief Calibration widget for 2 setpoint switch
  @author Bryan Godbolt <godbolt@ece.ualberta.ca>
  */
class SwitchCalibrator : public AbstractCalibrator
{
    Q_OBJECT
public:
    explicit SwitchCalibrator(QString title=QString(), QWidget *parent = 0);

    void set(const QVector<uint16_t> &data);

protected slots:
    void setDefault();
    void setToggled();

protected:
    QLabel *defaultPulseWidth;
    QLabel *toggledPulseWidth;

};

#endif // SWITCHCALIBRATOR_H

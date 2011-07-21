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
 *   @brief Calibration widget for 3 point airfoil servo
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef AIRFOILSERVOCALIBRATOR_H
#define AIRFOILSERVOCALIBRATOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>

#include "AbstractCalibrator.h"

/**
  @brief Calibration widget three setpoint control input.
   For the helicopter autopilot at UAlberta this is used for Aileron, Elevator, and Rudder channels.

  @author Bryan Godbolt <godbolt@ece.ualberta.ca>
  */
class AirfoilServoCalibrator : public AbstractCalibrator
{
    Q_OBJECT
public:
    enum AirfoilType {
        AILERON,
        ELEVATOR,
        RUDDER
    };

    explicit AirfoilServoCalibrator(AirfoilType type = AILERON, QWidget *parent = 0);

    /** @param data must have exaclty 3 elemets.  they are assumed to be low center high */
    void set(const QVector<uint16_t>& data);

protected slots:
    void setHigh();
    void setCenter();
    void setLow();

protected:
    QLabel *highPulseWidth;
    QLabel *centerPulseWidth;
    QLabel *lowPulseWidth;
};

#endif // AIRFOILSERVOCALIBRATOR_H

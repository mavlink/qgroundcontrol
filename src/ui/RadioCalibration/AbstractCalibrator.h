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
 *   @brief Common aspects of radio calibration widgets
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef ABSTRACTCALIBRATOR_H
#define ABSTRACTCALIBRATOR_H

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QVector>

#include <math.h>
#include <stdint.h>

/**
  @brief Holds the code which is common to all the radio calibration widgets.

  @author Bryan Godbolt <godbolt@ece.ualberta.ca>
  */
class AbstractCalibrator : public QWidget
{
    Q_OBJECT
public:
    explicit AbstractCalibrator(QWidget *parent = 0);
    ~AbstractCalibrator();

    /** Change the setpoints of the widget.  Used when
      changing the display from an external source (file/uav).
      @param data QVector of setpoints
      */
    virtual void set(const QVector<uint16_t>& data)=0;
signals:
    /** Announce a setpoint change.
      @param index setpoint number - 0 based in the current implementation
      @param value new value
      */
    void setpointChanged(int index, uint16_t value);

public slots:
    /** Slot to call when the relevant channel is updated
     @param raw current channel value
     */
    void channelChanged(uint16_t raw);

protected:
    /** Display the current pulse width */
    QLabel *pulseWidth;

    /** Log of the past few samples for use in averaging and finding extrema */
    QVector<uint16_t> *log;
    /** Find the maximum or minimum of the data log */
    uint16_t logExtrema();
    /** Find the average of the log */
    uint16_t logAverage();
};

#endif // ABSTRACTCALIBRATOR_H

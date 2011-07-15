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
 *   @brief Class to hold the calibration data
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef RADIOCALIBRATIONDATA_H
#define RADIOCALIBRATIONDATA_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QString>
#include <stdexcept>

#include <stdint.h>


/**
  @brief Class to hold the calibration data.
  @author Bryan Godbolt <godbolt@ece.ualberta.ca>
  */
class RadioCalibrationData : public QObject
{
    Q_OBJECT

public:
    explicit RadioCalibrationData();
    RadioCalibrationData(const RadioCalibrationData&);
    RadioCalibrationData(const QVector<uint16_t>& aileron,
                         const QVector<uint16_t>& elevator,
                         const QVector<uint16_t>& rudder,
                         const QVector<uint16_t>& gyro,
                         const QVector<uint16_t>& pitch,
                         const QVector<uint16_t>& throttle);
    ~RadioCalibrationData();

    enum RadioElement {
        AILERON=0,
        ELEVATOR,
        RUDDER,
        GYRO,
        PITCH,
        THROTTLE
    };

    const uint16_t* operator[](int i) const;
#ifdef _MSC_VER
    const QVector<uint16_t>& operator()(int i) const;
#else
    const QVector<uint16_t>& operator()(int i) const throw(std::out_of_range);
#endif
    void set(int element, int index, float value) {
        (*data)[element][index] = value;
    }

public slots:
    void setAileron(int index, uint16_t value) {
        set(AILERON, index, value);
    }
    void setElevator(int index, uint16_t value) {
        set(ELEVATOR, index, value);
    }
    void setRudder(int index, uint16_t value) {
        set(RUDDER, index, value);
    }
    void setGyro(int index, uint16_t value) {
        set(GYRO, index, value);
    }
    void setPitch(int index, uint16_t value) {
        set(PITCH, index, value);
    }
    void setThrottle(int index, uint16_t value) {
        set(THROTTLE, index, value);
    }

public:
    /// Creates a comma seperated list of the values for a particular element
    QString toString(const RadioElement element) const;

protected:
    QVector<QVector<uint16_t> > *data;

    void init(const QVector<float>& aileron,
              const QVector<float>& elevator,
              const QVector<float>& rudder,
              const QVector<float>& gyro,
              const QVector<float>& pitch,
              const QVector<float>& throttle);

};

#endif // RADIOCALIBRATIONDATA_H

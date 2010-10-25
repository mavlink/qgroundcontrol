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
    RadioCalibrationData(const QVector<float>& aileron,
                         const QVector<float>& elevator,
                         const QVector<float>& rudder,
                         const QVector<float>& gyro,
                         const QVector<float>& pitch,
                         const QVector<float>& throttle);
    ~RadioCalibrationData();

    enum RadioElement
    {
        AILERON=0,
        ELEVATOR,
        RUDDER,
        GYRO,
        PITCH,
        THROTTLE
    };

    const float* operator[](int i) const;
    const QVector<float>& operator()(int i) const;
    void set(int element, int index, float value) {(*data)[element][index] = value;}

public slots:
    void setAileron(int index, float value) {set(AILERON, index, value);}
    void setElevator(int index, float value) {set(ELEVATOR, index, value);}
    void setRudder(int index, float value) {set(RUDDER, index, value);}
    void setGyro(int index, float value) {set(GYRO, index, value);}
    void setPitch(int index, float value) {set(PITCH, index, value);}
    void setThrottle(int index, float value) {set(THROTTLE, index, value);}

public:
    /// Creates a comma seperated list of the values for a particular element
    QString toString(const RadioElement element) const;

protected:
    QVector<QVector<float> > *data;

    void init(const QVector<float>& aileron,
              const QVector<float>& elevator,
              const QVector<float>& rudder,
              const QVector<float>& gyro,
              const QVector<float>& pitch,
              const QVector<float>& throttle);

};

#endif // RADIOCALIBRATIONDATA_H

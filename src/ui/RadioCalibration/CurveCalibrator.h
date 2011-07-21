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
 *   @brief Calibration widget for 5 point inerpolated curve
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef CURVECALIBRATOR_H
#define CURVECALIBRATOR_H

#include <QWidget>
#include <QVector>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
//#include <qwt_array.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPen>
#include <QColor>
#include <QString>
#include <QSignalMapper>
#include <QDebug>

#include "AbstractCalibrator.h"

/**
  @brief Calibration widget for 5 point inerpolated curve.
  For the helicopter autopilot at UAlberta this is used for the throttle and pitch curves.
  */
class CurveCalibrator : public AbstractCalibrator
{
    Q_OBJECT
public:
    explicit CurveCalibrator(QString title = QString(), QWidget *parent = 0);
    ~CurveCalibrator();

    void set(const QVector<uint16_t> &data);

protected slots:
    void setSetpoint(int setpoint);

protected:
    QVector<uint16_t> *setpoints;
    QVector<uint16_t> *positions;
    /** Plot to display calibration curve */
    QwtPlot *plot;
    /** Curve object of calibration curve */
    QwtPlotCurve *curve;

    QSignalMapper *signalMapper;
};

#endif // CURVECALIBRATOR_H

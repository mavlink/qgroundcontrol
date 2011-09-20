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
 *   @brief Declaration of QGCRemoteControlView
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef QGCREMOTECONTROLVIEW_H
#define QGCREMOTECONTROLVIEW_H

#include <QWidget>
#include <QVector>
#include <QPushButton>

#include "RadioCalibration/RadioCalibrationWindow.h"

namespace Ui
{
class QGCRemoteControlView;
}

class QVBoxLayout;
class QLabel;
class QProgressBar;

class QGCRemoteControlView : public QWidget
{
    Q_OBJECT
public:
    QGCRemoteControlView(QWidget *parent = 0);
    ~QGCRemoteControlView();

public slots:
    void setUASId(int id);
    void setChannelRaw(int channelId, float raw);
    void setChannelScaled(int channelId, float normalized);
    void setRemoteRSSI(float rssiNormalized);
    void redraw();

protected slots:
    void appendChannelWidget(int channelId);

protected:
    void changeEvent(QEvent *e);
    int uasId;
    float rssi;
    bool updated;
    QVBoxLayout* channelLayout;
    QVector<int> raw;
    QVector<float> normalized;
    QVector<QLabel*> rawLabels;
    QVector<QProgressBar*> progressBars;
    QProgressBar* rssiBar;
    QLabel* nameLabel;
    QPushButton *calibrate;
    RadioCalibrationWindow *calibrationWindow;
    QTimer updateTimer;

private:
    Ui::QGCRemoteControlView *ui;
};

#endif // QGCREMOTECONTROLVIEW_H

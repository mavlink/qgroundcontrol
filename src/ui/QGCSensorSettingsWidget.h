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
 *   @brief Declaration of class QGCSensorSettingsWidget
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef QGCSENSORSETTINGSWIDGET_H
#define QGCSENSORSETTINGSWIDGET_H

#include <QWidget>

#include "UASInterface.h"

namespace Ui
{
class QGCSensorSettingsWidget;
}

class QGCSensorSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    QGCSensorSettingsWidget(UASInterface* uas, QWidget *parent = 0);
    ~QGCSensorSettingsWidget();

public slots:
    void delayedSendRawSensor(int rate);
    void delayedSendController(int rate);
    void delayedSendExtended(int rate);
    void delayedSendRC(int rate);
    void delayedSendPosition(int rate);
    void delayedSendExtra1(int rate);
    void delayedSendExtra2(int rate);
    void delayedSendExtra3(int rate);

protected:
    UASInterface* mav;
    QTimer delayedSendRawSensorTimer;
    QTimer delayedSendControllerTimer;
    QTimer delayedSendExtendedTimer;
    QTimer delayedSendRCTimer;
    QTimer delayedSendPositionTimer;
    QTimer delayedSendExtra1Timer;
    QTimer delayedSendExtra2Timer;
    QTimer delayedSendExtra3Timer;
    void changeEvent(QEvent *e);

protected slots:
    void sendRawSensor();
    void sendController();
    void sendExtended();
    void sendRC();
    void sendPosition();
    void sendExtra1();
    void sendExtra2();
    void sendExtra3();

private:
    Ui::QGCSensorSettingsWidget *ui;
};

#endif // QGCSENSORSETTINGSWIDGET_H

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
 *   @brief Declaration of class QGCPxImuFirmwareUpdate
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef QGCPXIMUFIRMWAREUPDATE_H
#define QGCPXIMUFIRMWAREUPDATE_H

#include <QWidget>

namespace Ui
{
class QGCPxImuFirmwareUpdate;
}

class QGCPxImuFirmwareUpdate : public QWidget
{
    Q_OBJECT
public:
    QGCPxImuFirmwareUpdate(QWidget *parent = 0);
    ~QGCPxImuFirmwareUpdate();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCPxImuFirmwareUpdate *ui;
};

#endif // QGCPXIMUFIRMWAREUPDATE_H

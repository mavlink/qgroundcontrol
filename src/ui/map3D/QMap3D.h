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
 *   @brief Definition of the class QMap3D.h
 *
 *   @author James Goppert <james.goppert@gmail.com>
 *
 */

#ifdef QGC_OSGEARTH_ENABLED

#ifndef QMAP3D_H
#define QMAP3D_H

#include "QOSGWidget.h"
#include "ui_QMap3D.h"

namespace Ui
{
class QMap3D;
}

class QMap3D : public QWidget, private Ui::QMap3D
{
    Q_OBJECT
public:
    QMap3D(QWidget * parent = 0, const char * name = 0, WindowFlags f = 0);
    ~QMap3D();
public slots:
    void on_pushButton_map_clicked();
    void on_pushButton_vehicle_clicked();
};

#endif // QMAP3D_H

#endif

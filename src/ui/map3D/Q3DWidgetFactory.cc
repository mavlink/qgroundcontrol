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
 *   @brief Definition of the class Q3DWidgetFactory.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "Q3DWidgetFactory.h"

#include "Pixhawk3DWidget.h"
#ifdef QGC_OSGEARTH_ENABLED
#include "QMap3D.h"
#endif

QPointer<QWidget>
Q3DWidgetFactory::get(const std::string& type, QWidget* parent)
{
    if (type == "PIXHAWK") {
        return QPointer<QWidget>(new Pixhawk3DWidget(parent));
    }
    else {
        return QPointer<QWidget>(new Q3DWidget(parent));
    }
}

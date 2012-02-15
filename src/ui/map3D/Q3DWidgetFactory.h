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

#ifndef Q3DWIDGETFACTORY_H
#define Q3DWIDGETFACTORY_H

#include <QPointer>

#include "Q3DWidget.h"

/**
 * @brief A factory which creates project-specific 3D widgets without
 *        specifying the exact widget class.
 */

class Q3DWidgetFactory
{
public:
    /**
     * @brief Returns a Q3DWidget instance of the appropriate type.
     * @param type String specifying the required type name.
     * @return A smart pointer to the Q3DWidget instance.
     */

    static QPointer<QWidget> get(const std::string& type, QWidget* parent);
};

#endif // Q3DWIDGETFACTORY_H

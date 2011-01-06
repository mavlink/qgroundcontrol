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
 *   @brief Definition of the class GCManipulator.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef GCMANIPULATOR_H
#define GCMANIPULATOR_H

#include <osgGA/TrackballManipulator>

class GCManipulator : public osgGA::TrackballManipulator
{
public:
    GCManipulator();

    void setMinZoomRange(double minZoomRange);

    virtual void move(double dx, double dy, double dz);

    /**
     * @brief Handle events.
     * @return True if event is handled; false otherwise.
     */
    virtual bool handle(const osgGA::GUIEventAdapter& ea,
                        osgGA::GUIActionAdapter& us);

protected:
    bool calcMovement(void);

    double _moveSensitivity;
    double _zoomSensitivity;
    double _minZoomRange;
};

#endif // GCMANIPULATOR_H

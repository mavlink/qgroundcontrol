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
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#include "GCManipulator.h"
#include <osg/Version>

GCManipulator::GCManipulator()
{
    _moveSensitivity = 0.05;
    _zoomSensitivity = 1.0;
    _minZoomRange = 2.0;
}

void
GCManipulator::setMinZoomRange(double minZoomRange)
{
    _minZoomRange = minZoomRange;
}

void
GCManipulator::move(double dx, double dy, double dz)
{
    _center += osg::Vec3d(dx, dy, dz);
}

bool
GCManipulator::handle(const osgGA::GUIEventAdapter& ea,
                      osgGA::GUIActionAdapter& us)
{
    using namespace osgGA;

    switch (ea.getEventType()) {
    case GUIEventAdapter::PUSH: {
        flushMouseEventStack();
        addMouseEvent(ea);
        if (calcMovement()) {
            us.requestRedraw();
        }
        us.requestContinuousUpdate(false);
        _thrown = false;
        return true;
    }

    case GUIEventAdapter::RELEASE: {
        if (ea.getButtonMask() == 0) {
            if (isMouseMoving()) {
                if (calcMovement()) {
                    us.requestRedraw();
                    us.requestContinuousUpdate(false);
                    _thrown = false;
                }
            } else {
                flushMouseEventStack();
                addMouseEvent(ea);
                if (calcMovement()) {
                    us.requestRedraw();
                }
                us.requestContinuousUpdate(false);
                _thrown = false;
            }

        } else {
            flushMouseEventStack();
            addMouseEvent(ea);
            if (calcMovement()) {
                us.requestRedraw();
            }
            us.requestContinuousUpdate(false);
            _thrown = false;
        }
        return true;
    }

    case GUIEventAdapter::DRAG: {
        addMouseEvent(ea);
        if (calcMovement()) {
            us.requestRedraw();
        }
        us.requestContinuousUpdate(false);
        _thrown = false;
        return true;
    }

    case GUIEventAdapter::SCROLL: {
        // zoom model
        double scale = 1.0;

        if (ea.getScrollingMotion() == GUIEventAdapter::SCROLL_UP) {
            scale -= _zoomSensitivity * 0.1;
        } else {
            scale += _zoomSensitivity * 0.1;
        }
        if (_distance * scale > _minZoomRange) {

            _distance *= scale;

        }

        return true;
    }

    case GUIEventAdapter::KEYDOWN:
        // pan model
        switch (ea.getKey()) {
        case GUIEventAdapter::KEY_Space: {
            flushMouseEventStack();
            _thrown = false;
            home(ea,us);
            us.requestRedraw();
            us.requestContinuousUpdate(false);
            return true;
        }
        case GUIEventAdapter::KEY_Left: {
            double scale = -_moveSensitivity * _distance;

            osg::Matrix rotation_matrix;
            rotation_matrix.makeRotate(_rotation);

            osg::Vec3d dv(scale, 0.0, 0.0);

            _center += dv * rotation_matrix;

            return true;
        }
        case GUIEventAdapter::KEY_Right: {
            double scale = _moveSensitivity * _distance;

            osg::Matrix rotation_matrix;
            rotation_matrix.makeRotate(_rotation);

            osg::Vec3d dv(scale, 0.0, 0.0);

            _center += dv * rotation_matrix;

            return true;
        }
        case GUIEventAdapter::KEY_Up: {
            double scale = _moveSensitivity * _distance;

            osg::Matrix rotation_matrix;
            rotation_matrix.makeRotate(_rotation);

            osg::Vec3d dv(0.0, scale, 0.0);

            _center += dv * rotation_matrix;

            return true;
        }
        case GUIEventAdapter::KEY_Down: {
            double scale = -_moveSensitivity * _distance;

            osg::Matrix rotation_matrix;
            rotation_matrix.makeRotate(_rotation);

            osg::Vec3d dv(0.0, scale, 0.0);

            _center += dv * rotation_matrix;

            return true;
        }
        return false;
        }

    case GUIEventAdapter::FRAME:
        if (_thrown) {
            if (calcMovement()) {
                us.requestRedraw();
            }
        }
        return false;

    default:
        return false;
    }
}


bool
GCManipulator::calcMovement(void)
{
    using namespace osgGA;

    // return if less then two events have been added.
    if (_ga_t0.get() == NULL || _ga_t1.get() == NULL) {
        return false;
    }

    double dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
    double dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();

    // return if there is no movement.
    if (dx == 0.0 && dy == 0.0) {
        return false;
    }

    unsigned int buttonMask = _ga_t1->getButtonMask();
    if (buttonMask == GUIEventAdapter::LEFT_MOUSE_BUTTON) {
        // rotate camera
#if ((OPENSCENEGRAPH_MAJOR_VERSION == 2) & (OPENSCENEGRAPH_MINOR_VERSION > 8)) | (OPENSCENEGRAPH_MAJOR_VERSION > 2)
        osg::Vec3d axis;
#else
        osg::Vec3 axis;
#endif
        float angle;

        float px0 = _ga_t0->getXnormalized();
        float py0 = _ga_t0->getYnormalized();

        float px1 = _ga_t1->getXnormalized();
        float py1 = _ga_t1->getYnormalized();

        trackball(axis, angle, px1, py1, px0, py0);

        osg::Quat new_rotate;
        new_rotate.makeRotate(angle, axis);

        _rotation = _rotation * new_rotate;

        return true;

    } else if (buttonMask == GUIEventAdapter::MIDDLE_MOUSE_BUTTON ||
               buttonMask == (GUIEventAdapter::LEFT_MOUSE_BUTTON |
                              GUIEventAdapter::RIGHT_MOUSE_BUTTON)) {
        // pan model
        double scale = -_moveSensitivity * _distance;

        osg::Matrix rotation_matrix;
        rotation_matrix.makeRotate(_rotation);

        osg::Vec3d dv(dx * scale, dy * scale, 0.0);

        _center += dv * rotation_matrix;

        return true;

    } else if (buttonMask == GUIEventAdapter::RIGHT_MOUSE_BUTTON) {
        // zoom model
        double scale = 1.0 + dy * _zoomSensitivity;
        if (_distance * scale > _minZoomRange) {

            _distance *= scale;

        }
        return true;
    }

    return false;
}

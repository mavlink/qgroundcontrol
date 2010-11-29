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

#include "GCManipulator.h"

GCManipulator::GCManipulator()
{
    _moveSensitivity = 0.05f;
    _zoomSensitivity = 1.0f;
    _minZoomRange = 2.0f;
}

void
GCManipulator::setMinZoomRange(float minZoomRange)
{
    _minZoomRange = minZoomRange;
}

void
GCManipulator::move(float dx, float dy, float dz)
{
    _center += osg::Vec3(dx, dy, dz);
}

bool
GCManipulator::handle(const osgGA::GUIEventAdapter& ea,
                      osgGA::GUIActionAdapter& us)
{
    using namespace osgGA;

    switch (ea.getEventType())
    {
    case GUIEventAdapter::PUSH:
        {
            flushMouseEventStack();
            addMouseEvent(ea);
            if (calcMovement())
            {
                us.requestRedraw();
            }
            us.requestContinuousUpdate(false);
            _thrown = false;
            return true;
        }

    case GUIEventAdapter::RELEASE:
        {
            if (ea.getButtonMask() == 0)
            {
                if (isMouseMoving())
                {
                    if (calcMovement())
                    {
                        us.requestRedraw();
                        us.requestContinuousUpdate(true);
                        _thrown = true;
                    }
                }
                else
                {
                    flushMouseEventStack();
                    addMouseEvent(ea);
                    if (calcMovement())
                    {
                        us.requestRedraw();
                    }
                    us.requestContinuousUpdate(false);
                    _thrown = false;
                }

            }
            else
            {
                flushMouseEventStack();
                addMouseEvent(ea);
                if (calcMovement())
                {
                    us.requestRedraw();
                }
                us.requestContinuousUpdate(false);
                _thrown = false;
            }
            return true;
        }

    case GUIEventAdapter::DRAG:
        {
            addMouseEvent(ea);
            if (calcMovement())
            {
                us.requestRedraw();
            }
            us.requestContinuousUpdate(false);
            _thrown = false;
            return true;
        }

    case GUIEventAdapter::SCROLL:
        {
            // zoom model
            float scale = 1.0f;

            if (ea.getScrollingMotion() == GUIEventAdapter::SCROLL_UP)
            {
                scale -= _zoomSensitivity * 0.1f;
            }
            else
            {
                scale += _zoomSensitivity * 0.1f;
            }
            if (_distance * scale > _minZoomRange)
            {

                _distance *= scale;

            }

            return true;
        }

    case GUIEventAdapter::KEYDOWN:
        // pan model
        switch (ea.getKey())
        {
        case GUIEventAdapter::KEY_Space:
            {
                flushMouseEventStack();
                _thrown = false;
                home(ea,us);
                us.requestRedraw();
                us.requestContinuousUpdate(false);
                return true;
            }
        case GUIEventAdapter::KEY_Left:
            {
                float scale = -_moveSensitivity * _distance;

                osg::Matrix rotation_matrix;
                rotation_matrix.makeRotate(_rotation);

                osg::Vec3 dv(scale, 0.0f, 0.0f);

                _center += dv * rotation_matrix;

                return true;
            }
        case GUIEventAdapter::KEY_Right:
            {
                float scale = _moveSensitivity * _distance;

                osg::Matrix rotation_matrix;
                rotation_matrix.makeRotate(_rotation);

                osg::Vec3 dv(scale, 0.0f, 0.0f);

                _center += dv * rotation_matrix;

                return true;
            }
        case GUIEventAdapter::KEY_Up:
            {
                float scale = _moveSensitivity * _distance;

                osg::Matrix rotation_matrix;
                rotation_matrix.makeRotate(_rotation);

                osg::Vec3 dv(0.0f, scale, 0.0f);

                _center += dv * rotation_matrix;

                return true;
            }
        case GUIEventAdapter::KEY_Down:
            {
                float scale = -_moveSensitivity * _distance;

                osg::Matrix rotation_matrix;
                rotation_matrix.makeRotate(_rotation);

                osg::Vec3 dv(0.0f, scale, 0.0f);

                _center += dv * rotation_matrix;

                return true;
            }
            return false;
        }

    case GUIEventAdapter::FRAME:
        if (_thrown)
        {
            if (calcMovement())
            {
                us.requestRedraw();
            }
        }
        return false;

    default:
        return false;
    }
}


bool
GCManipulator::calcMovement()
{
    using namespace osgGA;

    // return if less then two events have been added.
    if (_ga_t0.get() == NULL || _ga_t1.get() == NULL)
    {
        return false;
    }

    float dx = _ga_t0->getXnormalized() - _ga_t1->getXnormalized();
    float dy = _ga_t0->getYnormalized() - _ga_t1->getYnormalized();

    // return if there is no movement.
    if (dx == 0.0f && dy == 0.0f)
    {
        return false;
    }

    unsigned int buttonMask = _ga_t1->getButtonMask();
    if (buttonMask == GUIEventAdapter::LEFT_MOUSE_BUTTON)
    {
        // rotate camera
        osg::Vec3d axis;
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

    }
    else if (buttonMask == GUIEventAdapter::MIDDLE_MOUSE_BUTTON ||
             buttonMask == (GUIEventAdapter::LEFT_MOUSE_BUTTON |
                            GUIEventAdapter::RIGHT_MOUSE_BUTTON))
    {
        // pan model
        float scale = -_moveSensitivity * _distance;

        osg::Matrix rotation_matrix;
        rotation_matrix.makeRotate(_rotation);

        osg::Vec3 dv(dx * scale, dy * scale, 0.0f);

        _center += dv * rotation_matrix;

        return true;

    }
    else if (buttonMask == GUIEventAdapter::RIGHT_MOUSE_BUTTON)
    {
        // zoom model
        float scale = 1.0f + dy * _zoomSensitivity;
        if (_distance * scale > _minZoomRange)
        {

            _distance *= scale;

        }
        return true;
    }

    return false;
}

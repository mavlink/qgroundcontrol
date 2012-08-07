/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSGGA_DRIVEMANIPULATOR
#define OSGGA_DRIVEMANIPULATOR 1

#include <osgGA/CameraManipulator>
#include <osg/Quat>

namespace osgGA{

/**
DriveManipulator is a camera manipulator which provides drive-like
functionality. By default, the left mouse button accelerates, the right
mouse button decelerates, and the middle mouse button (or left and
right simultaneously) stops dead.
*/

class OSGGA_EXPORT DriveManipulator : public CameraManipulator
{
    public:

        DriveManipulator();

        virtual const char* className() const { return "Drive"; }

        /** Get the position of the matrix manipulator using a 4x4 Matrix.*/
        virtual void setByMatrix(const osg::Matrixd& matrix);

        /** Set the position of the matrix manipulator using a 4x4 Matrix.*/
        virtual void setByInverseMatrix(const osg::Matrixd& matrix) { setByMatrix(osg::Matrixd::inverse(matrix)); }

        /** Get the position of the manipulator as 4x4 Matrix.*/
        virtual osg::Matrixd getMatrix() const;

        /** Get the position of the manipulator as a inverse matrix of the manipulator, typically used as a model view matrix.*/
        virtual osg::Matrixd getInverseMatrix() const;

        virtual void setNode(osg::Node*);

        virtual const osg::Node* getNode() const;

        virtual osg::Node* getNode();

        virtual void computeHomePosition();

        virtual void home(const GUIEventAdapter& ea,GUIActionAdapter& us);

        virtual void init(const GUIEventAdapter& ea,GUIActionAdapter& us);

        virtual bool handle(const GUIEventAdapter& ea,GUIActionAdapter& us);

        /** Get the keyboard and mouse usage of this manipulator.*/
        virtual void getUsage(osg::ApplicationUsage& usage) const;

        void setModelScale( double in_ms ) { _modelScale = in_ms; }
        double getModelScale() const { return _modelScale; }

        void setVelocity( double in_vel ) { _velocity = in_vel; }
        double getVelocity() const { return _velocity; }

        void setHeight( double in_h ) { _height = in_h; }
        double getHeight() const { return _height; }

    protected:

        virtual ~DriveManipulator();

        bool intersect(const osg::Vec3d& start, const osg::Vec3d& end, osg::Vec3d& intersection, osg::Vec3d& normal) const;

        /** Reset the internal GUIEvent stack.*/
        void flushMouseEventStack();

        /** Add the current mouse GUIEvent to internal stack.*/
        void addMouseEvent(const GUIEventAdapter& ea);

        void computePosition(const osg::Vec3d& eye,const osg::Vec3d& lv,const osg::Vec3d& up);

        /** For the given mouse movement calculate the movement of the camera.
          * Return true if camera has moved and a redraw is required.
          */
        bool calcMovement();

        // Internal event stack comprising last two mouse events.
        osg::ref_ptr<const GUIEventAdapter> _ga_t1;
        osg::ref_ptr<const GUIEventAdapter> _ga_t0;

        osg::observer_ptr<osg::Node>    _node;

        double _modelScale;
        double _velocity;
        double _height;
        double _buffer;

        enum SpeedControlMode {
                USE_MOUSE_Y_FOR_SPEED,
                USE_MOUSE_BUTTONS_FOR_SPEED
        };

        SpeedControlMode _speedMode;

        osg::Vec3d   _eye;
        osg::Quat    _rotation;
        double       _pitch;
        double       _distance;

        bool        _pitchUpKeyPressed;
        bool        _pitchDownKeyPressed;
};

}

#endif

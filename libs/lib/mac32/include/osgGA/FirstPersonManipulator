/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2010 Robert Osfield
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
 *
 * FirstPersonManipulator code Copyright (C) 2010 PCJohn (Jan Peciva)
 * while some pieces of code were taken from OSG.
 * Thanks to company Cadwork (www.cadwork.ch) and
 * Brno University of Technology (www.fit.vutbr.cz) for open-sourcing this work.
*/

#ifndef OSGGA_FIRST_PERSON_MANIPULATOR
#define OSGGA_FIRST_PERSON_MANIPULATOR 1

#include <osgGA/StandardManipulator>


namespace osgGA {


/** FirstPersonManipulator is base class for camera control based on position
    and orientation of camera, like walk, drive, and flight manipulators. */
class OSGGA_EXPORT FirstPersonManipulator : public StandardManipulator
{
        typedef StandardManipulator inherited;

    public:

        FirstPersonManipulator( int flags = DEFAULT_SETTINGS );
        FirstPersonManipulator( const FirstPersonManipulator& fpm,
                                const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY );

        META_Object( osgGA, FirstPersonManipulator );

        virtual void setByMatrix( const osg::Matrixd& matrix );
        virtual void setByInverseMatrix( const osg::Matrixd& matrix );
        virtual osg::Matrixd getMatrix() const;
        virtual osg::Matrixd getInverseMatrix() const;

        virtual void setTransformation( const osg::Vec3d& eye, const osg::Quat& rotation );
        virtual void setTransformation( const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up );
        virtual void getTransformation( osg::Vec3d& eye, osg::Quat& rotation ) const;
        virtual void getTransformation( osg::Vec3d& eye, osg::Vec3d& center, osg::Vec3d& up ) const;

        virtual void setVelocity( const double& velocity );
        inline double getVelocity() const;
        virtual void setAcceleration( const double& acceleration, bool relativeToModelSize = false );
        double getAcceleration( bool *relativeToModelSize = NULL ) const;
        virtual void setMaxVelocity( const double& maxVelocity, bool relativeToModelSize = false );
        double getMaxVelocity( bool *relativeToModelSize = NULL ) const;

        virtual void setWheelMovement( const double& wheelMovement, bool relativeToModelSize = false );
        double getWheelMovement( bool *relativeToModelSize = NULL ) const;

        virtual void home( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us );
        virtual void home( double );

        virtual void init( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us );

    protected:

        virtual bool handleMouseWheel( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us );

        virtual bool performMovementLeftMouseButton( const double eventTimeDelta, const double dx, const double dy );
        virtual bool performMouseDeltaMovement( const float dx, const float dy );
        virtual void applyAnimationStep( const double currentProgress, const double prevProgress );
        virtual bool startAnimationByMousePointerIntersection( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us );

        void moveForward( const double distance );
        void moveForward( const osg::Quat& rotation, const double distance );
        void moveRight( const double distance );
        void moveUp( const double distance );

        osg::Vec3d _eye;
        osg::Quat  _rotation;
        double _velocity;

        double _acceleration;
        static int _accelerationFlagIndex;
        double _maxVelocity;
        static int _maxVelocityFlagIndex;
        double _wheelMovement;
        static int _wheelMovementFlagIndex;

        class FirstPersonAnimationData : public AnimationData {
        public:
            osg::Quat _startRot;
            osg::Quat _targetRot;
            void start( const osg::Quat& startRotation, const osg::Quat& targetRotation, const double startTime );
        };
        virtual void allocAnimationData() { _animationData = new FirstPersonAnimationData(); }
};


//
//  inline methods
//

/// Returns velocity.
double FirstPersonManipulator::getVelocity() const  { return _velocity; }


}

#endif /* OSGGA_FIRST_PERSON_MANIPULATOR */

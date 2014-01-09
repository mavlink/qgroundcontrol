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
 * OrbitManipulator code Copyright (C) 2010 PCJohn (Jan Peciva)
 * while some pieces of code were taken from OSG.
 * Thanks to company Cadwork (www.cadwork.ch) and
 * Brno University of Technology (www.fit.vutbr.cz) for open-sourcing this work.
*/

#ifndef OSGGA_ORBIT_MANIPULATOR
#define OSGGA_ORBIT_MANIPULATOR 1

#include <osgGA/StandardManipulator>


namespace osgGA {


/** OrbitManipulator is base class for camera control based on focal center,
    distance from the center, and orientation of distance vector to the eye.
    This is the base class for trackball style manipulators.*/
class OSGGA_EXPORT OrbitManipulator : public StandardManipulator
{
        typedef StandardManipulator inherited;

    public:

        OrbitManipulator( int flags = DEFAULT_SETTINGS );
        OrbitManipulator( const OrbitManipulator& om,
                          const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY );

        META_Object( osgGA, OrbitManipulator );

        virtual void setByMatrix( const osg::Matrixd& matrix );
        virtual void setByInverseMatrix( const osg::Matrixd& matrix );
        virtual osg::Matrixd getMatrix() const;
        virtual osg::Matrixd getInverseMatrix() const;

        virtual void setTransformation( const osg::Vec3d& eye, const osg::Quat& rotation );
        virtual void setTransformation( const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up );
        virtual void getTransformation( osg::Vec3d& eye, osg::Quat& rotation ) const;
        virtual void getTransformation( osg::Vec3d& eye, osg::Vec3d& center, osg::Vec3d& up ) const;

        void setHeading( double azimuth );
        double getHeading() const;
        void setElevation( double elevation );
        double getElevation() const;

        virtual void setCenter( const osg::Vec3d& center );
        const osg::Vec3d& getCenter() const;
        virtual void setRotation( const osg::Quat& rotation );
        const osg::Quat& getRotation() const;
        virtual void setDistance( double distance );
        double getDistance() const;

        virtual void setTrackballSize( const double& size );
        inline double getTrackballSize() const;
        virtual void setWheelZoomFactor( double wheelZoomFactor );
        inline double getWheelZoomFactor() const;

        virtual void setMinimumDistance( const double& minimumDistance, bool relativeToModelSize = false );
        double getMinimumDistance( bool *relativeToModelSize = NULL ) const;

        virtual osgUtil::SceneView::FusionDistanceMode getFusionDistanceMode() const;
        virtual float getFusionDistanceValue() const;

    protected:

        virtual bool handleMouseWheel( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us );

        virtual bool performMovementLeftMouseButton( const double eventTimeDelta, const double dx, const double dy );
        virtual bool performMovementMiddleMouseButton( const double eventTimeDelta, const double dx, const double dy );
        virtual bool performMovementRightMouseButton( const double eventTimeDelta, const double dx, const double dy );
        virtual bool performMouseDeltaMovement( const float dx, const float dy );
        virtual void applyAnimationStep( const double currentProgress, const double prevProgress );

        virtual void rotateTrackball( const float px0, const float py0,
                                      const float px1, const float py1, const float scale );
        virtual void rotateWithFixedVertical( const float dx, const float dy );
        virtual void rotateWithFixedVertical( const float dx, const float dy, const osg::Vec3f& up );
        virtual void panModel( const float dx, const float dy, const float dz = 0.f );
        virtual void zoomModel( const float dy, bool pushForwardIfNeeded = true );
        void trackball( osg::Vec3d& axis, float& angle, float p1x, float p1y, float p2x, float p2y );
        float tb_project_to_sphere( float r, float x, float y );
        virtual bool startAnimationByMousePointerIntersection( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us );

        osg::Vec3d _center;
        osg::Quat  _rotation;
        double     _distance;

        double _trackballSize;
        double _wheelZoomFactor;

        double _minimumDistance;
        static int _minimumDistanceFlagIndex;

        class OrbitAnimationData : public AnimationData {
        public:
            osg::Vec3d _movement;
            void start( const osg::Vec3d& movement, const double startTime );
        };
        virtual void allocAnimationData() { _animationData = new OrbitAnimationData(); }
};


//
//  inline functions
//

/** Get the size of the trackball relative to the model size. */
inline double OrbitManipulator::getTrackballSize() const  { return _trackballSize; }
/** Get the mouse wheel zoom factor.*/
inline double OrbitManipulator::getWheelZoomFactor() const  { return _wheelZoomFactor; }


}

#endif /* OSGGA_ORBIT_MANIPULATOR */

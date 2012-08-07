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

#ifndef OSGGA_NODE_TRACKER_MANIPULATOR
#define OSGGA_NODE_TRACKER_MANIPULATOR 1

#include <osgGA/OrbitManipulator>
#include <osg/ObserverNodePath>


namespace osgGA {


class OSGGA_EXPORT NodeTrackerManipulator : public OrbitManipulator
{
        typedef OrbitManipulator inherited;

    public:

        NodeTrackerManipulator( int flags = DEFAULT_SETTINGS );

        NodeTrackerManipulator( const NodeTrackerManipulator& om,
                                const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY );

        META_Object( osgGA, NodeTrackerManipulator );

        void setTrackNodePath(const osg::NodePath& nodePath);
        void setTrackNodePath(const osg::ObserverNodePath& nodePath) { _trackNodePath = nodePath; }
        osg::ObserverNodePath& getTrackNodePath() { return _trackNodePath; }

        void setTrackNode(osg::Node* node);
        osg::Node* getTrackNode() { osg::NodePath nodePath; return _trackNodePath.getNodePath(nodePath) && !nodePath.empty() ? nodePath.back() : 0; }
        const osg::Node* getTrackNode() const { osg::NodePath nodePath; return _trackNodePath.getNodePath(nodePath) && !nodePath.empty()  ? nodePath.back() : 0; }

        enum TrackerMode
        {
            /** Track the center of the node's bounding sphere, but not rotations of the node.
              * For databases which have a CoordinateSystemNode, the orientation is kept relative the coordinate frame if the center of the node.
              */
            NODE_CENTER,
            /** Track the center of the node's bounding sphere, and the azimuth rotation (about the z axis of the current coordinate frame).
              * For databases which have a CoordinateSystemNode, the orientation is kept relative the coordinate frame if the center of the node.
              */
            NODE_CENTER_AND_AZIM,
            /** Tack the center of the node's bounding sphere, and the all rotations of the node.
              */
            NODE_CENTER_AND_ROTATION
        };

        void setTrackerMode(TrackerMode mode);
        TrackerMode getTrackerMode() const { return _trackerMode; }


        enum RotationMode
        {
            /** Use a trackball style manipulation of the view direction w.r.t the tracked orientation.
              */
            TRACKBALL,
            /** Allow the elevation and azimuth angles to be adjust w.r.t the tracked orientation.
              */
            ELEVATION_AZIM
        };

        void setRotationMode(RotationMode mode);
        RotationMode getRotationMode() const;


        virtual void setByMatrix(const osg::Matrixd& matrix);
        virtual osg::Matrixd getMatrix() const;
        virtual osg::Matrixd getInverseMatrix() const;

        virtual void setNode(osg::Node*);

        virtual void computeHomePosition();

    protected:

        virtual bool performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy);
        virtual bool performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy);
        virtual bool performMovementRightMouseButton(const double eventTimeDelta, const double dx, const double dy);

        void computeNodeWorldToLocal(osg::Matrixd& worldToLocal) const;
        void computeNodeLocalToWorld(osg::Matrixd& localToWorld) const;

        void computeNodeCenterAndRotation(osg::Vec3d& center, osg::Quat& rotation) const;

        void computePosition(const osg::Vec3d& eye,const osg::Vec3d& lv,const osg::Vec3d& up);


        osg::ObserverNodePath   _trackNodePath;
        TrackerMode             _trackerMode;

};

}

#endif /* OSGGA_NODE_TRACKER_MANIPULATOR */

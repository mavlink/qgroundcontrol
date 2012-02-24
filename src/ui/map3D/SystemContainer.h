#ifndef SYSTEMCONTAINER_H
#define SYSTEMCONTAINER_H

#include <osg/Geode>
#include <QMap>
#include <QVarLengthArray>
#include <QVector3D>
#include <QVector4D>

#include "ImageWindowGeode.h"
#include "WaypointGroupNode.h"

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
#include "GLOverlayGeode.h"
#include "ObstacleGroupNode.h"
#endif

class SystemContainer
{
public:
    SystemContainer();

    QVector3D& gpsLocalOrigin(void);

    QVector4D& target(void);

    QVector< osg::ref_ptr<osg::Node> >& models(void);

    QMap<int, QVector<osg::Vec3d> >& trailMap(void);
    QMap<int, int>& trailIndexMap(void);

    osg::ref_ptr<ImageWindowGeode>& depthImageNode(void);
    osg::ref_ptr<osg::Geode>& localGridNode(void);
    osg::ref_ptr<osg::Node>& modelNode(void);
    osg::ref_ptr<osg::Group>& orientationNode(void);
    osg::ref_ptr<osg::Geode>& pointCloudNode(void);
    osg::ref_ptr<ImageWindowGeode>& rgbImageNode(void);
    osg::ref_ptr<osg::Group>& setpointGroupNode(void);
    osg::ref_ptr<osg::Node>& targetNode(void);
    osg::ref_ptr<osg::Geode>& trailNode(void);
    osg::ref_ptr<WaypointGroupNode>& waypointGroupNode(void);
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    osg::ref_ptr<ObstacleGroupNode>& obstacleGroupNode(void);
    QMap<QString,osg::ref_ptr<GLOverlayGeode> >& overlayNodeMap(void);
    osg::ref_ptr<osg::Geode>& plannedPathNode(void);
#endif

private:
    QVector3D mGPSLocalOrigin;
    QVector4D mTarget;

    QVector< osg::ref_ptr<osg::Node> > mModels;

    // map component id to component-specific trail
    QMap<int, QVector<osg::Vec3d> > mTrailMap;
    QMap<int, int> mTrailIndexMap;

    // osg structures
    osg::ref_ptr<ImageWindowGeode> mDepthImageNode;
    osg::ref_ptr<osg::Geode> mLocalGridNode;
    osg::ref_ptr<osg::Node> mModelNode;
    osg::ref_ptr<osg::Group> mOrientationNode;
    osg::ref_ptr<osg::Geode> mPointCloudNode;
    osg::ref_ptr<ImageWindowGeode> mRGBImageNode;
    osg::ref_ptr<osg::Group> mSetpointGroupNode;
    osg::ref_ptr<osg::Node> mTargetNode;
    osg::ref_ptr<osg::Geode> mTrailNode;
    osg::ref_ptr<WaypointGroupNode> mWaypointGroupNode;
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    osg::ref_ptr<ObstacleGroupNode> mObstacleGroupNode;
    QMap<QString,osg::ref_ptr<GLOverlayGeode> > mOverlayNodeMap;
    osg::ref_ptr<osg::Geode> mPlannedPathNode;
#endif
};

#endif // SYSTEMCONTAINER_H

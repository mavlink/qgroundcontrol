#include "SystemContainer.h"

#include <osg/PositionAttitudeTransform>

SystemContainer::SystemContainer()
{

}

QVector3D&
SystemContainer::gpsLocalOrigin(void)
{
    return mGPSLocalOrigin;
}

QVector4D&
SystemContainer::target(void)
{
    return mTarget;
}

QVector< osg::ref_ptr<osg::Node> >&
SystemContainer::models(void)
{
    return mModels;
}

QMap<int, QVector<osg::Vec3d> >&
SystemContainer::trailMap(void)
{
    return mTrailMap;
}

QMap<int, int>&
SystemContainer::trailIndexMap(void)
{
    return mTrailIndexMap;
}

osg::ref_ptr<ImageWindowGeode>&
SystemContainer::depthImageNode(void)
{
    return mDepthImageNode;
}

osg::ref_ptr<osg::Geode>&
SystemContainer::localGridNode(void)
{
    return mLocalGridNode;
}

osg::ref_ptr<osg::Node>&
SystemContainer::modelNode(void)
{
    return mModelNode;
}

osg::ref_ptr<osg::Group>&
SystemContainer::orientationNode(void)
{
    return mOrientationNode;
}

osg::ref_ptr<osg::Geode>&
SystemContainer::pointCloudNode(void)
{
    return mPointCloudNode;
}

osg::ref_ptr<ImageWindowGeode>&
SystemContainer::rgbImageNode(void)
{
    return mRGBImageNode;
}

osg::ref_ptr<osg::Group>&
SystemContainer::setpointGroupNode(void)
{
    return mSetpointGroupNode;
}

osg::ref_ptr<osg::Node>&
SystemContainer::targetNode(void)
{
    return mTargetNode;
}

osg::ref_ptr<osg::Geode>&
SystemContainer::trailNode(void)
{
    return mTrailNode;
}

osg::ref_ptr<WaypointGroupNode>&
SystemContainer::waypointGroupNode(void)
{
    return mWaypointGroupNode;
}

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)

osg::ref_ptr<ObstacleGroupNode>&
SystemContainer::obstacleGroupNode(void)
{
    return mObstacleGroupNode;
}

QMap<QString,osg::ref_ptr<GLOverlayGeode> >&
SystemContainer::overlayNodeMap(void)
{
    return mOverlayNodeMap;
}

osg::ref_ptr<osg::Geode>&
SystemContainer::plannedPathNode(void)
{
    return mPlannedPathNode;
}

#endif

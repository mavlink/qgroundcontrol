#ifndef SYSTEMGROUPNODE_H
#define SYSTEMGROUPNODE_H

#include <osg/PositionAttitudeTransform>
#include <osg/Switch>

class SystemGroupNode : public osg::Group
{
public:
    SystemGroupNode();

    osg::ref_ptr<osg::Switch>& allocentricMap(void);
    osg::ref_ptr<osg::Switch>& rollingMap(void);
    osg::ref_ptr<osg::Switch>& egocentricMap(void);
    osg::ref_ptr<osg::PositionAttitudeTransform>& position(void);
    osg::ref_ptr<osg::PositionAttitudeTransform>& attitude(void);

private:
    osg::ref_ptr<osg::Node> createAxisGeode(void);

    osg::ref_ptr<osg::Switch> mAllocentricMap;
    osg::ref_ptr<osg::Switch> mRollingMap;
    osg::ref_ptr<osg::Switch> mEgocentricMap;
    osg::ref_ptr<osg::PositionAttitudeTransform> mPosition;
    osg::ref_ptr<osg::PositionAttitudeTransform> mAttitude;
};

#endif // SYSTEMGROUPNODE_H

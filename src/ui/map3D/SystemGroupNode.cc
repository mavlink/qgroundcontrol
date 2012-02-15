#include "SystemGroupNode.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>

SystemGroupNode::SystemGroupNode()
 : mAllocentricMap(new osg::Switch())
 , mRollingMap(new osg::Switch())
 , mEgocentricMap(new osg::Switch())
 , mPosition(new osg::PositionAttitudeTransform())
 , mAttitude(new osg::PositionAttitudeTransform())
{
    addChild(mAllocentricMap);

    // set up various maps
    // allocentric - world map
    // rolling - map aligned to the world axes and centered on the robot
    // egocentric - vehicle-centric map
    mAllocentricMap->addChild(mPosition);
    mPosition->addChild(mRollingMap);
    mRollingMap->addChild(mAttitude);
    mAttitude->addChild(mEgocentricMap);

    // set up robot
    mEgocentricMap->addChild(createAxisGeode());
}

osg::ref_ptr<osg::Switch>&
SystemGroupNode::allocentricMap(void)
{
    return mAllocentricMap;
}

osg::ref_ptr<osg::Switch>&
SystemGroupNode::rollingMap(void)
{
    return mRollingMap;
}

osg::ref_ptr<osg::Switch>&
SystemGroupNode::egocentricMap(void)
{
    return mEgocentricMap;
}

osg::ref_ptr<osg::PositionAttitudeTransform>&
SystemGroupNode::position(void)
{
    return mPosition;
}

osg::ref_ptr<osg::PositionAttitudeTransform>&
SystemGroupNode::attitude(void)
{
    return mAttitude;
}

osg::ref_ptr<osg::Node>
SystemGroupNode::createAxisGeode(void)
{
    // draw x,y,z-axes
    osg::ref_ptr<osg::Geode> geode(new osg::Geode());
    osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry());
    geode->addDrawable(geometry.get());

    osg::ref_ptr<osg::Vec3Array> coords(new osg::Vec3Array(6));
    (*coords)[0] = (*coords)[2] = (*coords)[4] =
                                      osg::Vec3(0.0f, 0.0f, 0.0f);
    (*coords)[1] = osg::Vec3(0.0f, 0.3f, 0.0f);
    (*coords)[3] = osg::Vec3(0.15f, 0.0f, 0.0f);
    (*coords)[5] = osg::Vec3(0.0f, 0.0f, -0.15f);

    geometry->setVertexArray(coords);

    osg::Vec4 redColor(1.0f, 0.0f, 0.0f, 0.0f);
    osg::Vec4 greenColor(0.0f, 1.0f, 0.0f, 0.0f);
    osg::Vec4 blueColor(0.0f, 0.0f, 1.0f, 0.0f);

    osg::ref_ptr<osg::Vec4Array> color(new osg::Vec4Array(6));
    (*color)[0] = redColor;
    (*color)[1] = redColor;
    (*color)[2] = greenColor;
    (*color)[3] = greenColor;
    (*color)[4] = blueColor;
    (*color)[5] = blueColor;

    geometry->setColorArray(color);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 6));

    osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth());
    linewidth->setWidth(3.0f);
    stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geometry->setStateSet(stateset);

    return geode;
}

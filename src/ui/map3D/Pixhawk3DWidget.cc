///*=====================================================================
//
//QGroundControl Open Source Ground Control Station
//
//(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
//
//This file is part of the QGROUNDCONTROL project
//
//    QGROUNDCONTROL is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    QGROUNDCONTROL is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
//
//======================================================================*/

/**
 * @file
 *   @brief Definition of the class Pixhawk3DWidget.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "Pixhawk3DWidget.h"

#include <sstream>

#include <osg/Geode>
#include <osg/LineWidth>
#include <osg/ShapeDrawable>

#include "PixhawkCheetahGeode.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "QGC.h"

Pixhawk3DWidget::Pixhawk3DWidget(QWidget* parent)
     : Q3DWidget(parent)
     , uas(NULL)
     , displayGrid(true)
     , displayTrail(false)
     , displayTarget(false)
     , displayWaypoints(true)
     , followCamera(true)
     , lastRobotX(0.0f)
     , lastRobotY(0.0f)
     , lastRobotZ(0.0f)
{
    setCameraParams(2.0f, 30.0f, 0.01f, 10000.0f);
    init(15.0f);

    // generate Pixhawk Cheetah model
    egocentricMap->addChild(PixhawkCheetahGeode::instance());

    // generate grid model
    gridNode = createGrid();
    rollingMap->addChild(gridNode);

    // generate empty trail model
    trailNode = createTrail();
    rollingMap->addChild(trailNode);

    // generate map model
    mapNode = createMap();
    root->addChild(mapNode);

    // generate target model
    allocentricMap->addChild(createTarget());

    // generate waypoint model
    waypointsNode = createWaypoints();
    rollingMap->addChild(waypointsNode);

    setupHUD();

    buildLayout();

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));
}

Pixhawk3DWidget::~Pixhawk3DWidget()
{

}

void
Pixhawk3DWidget::buildLayout(void)
{
    QCheckBox* gridCheckBox = new QCheckBox(this);
    gridCheckBox->setText("Grid");
    gridCheckBox->setChecked(displayGrid);

    QCheckBox* trailCheckBox = new QCheckBox(this);
    trailCheckBox->setText("Trail");
    trailCheckBox->setChecked(displayTrail);

    QCheckBox* waypointsCheckBox = new QCheckBox(this);
    waypointsCheckBox->setText("Waypoints");
    waypointsCheckBox->setChecked(displayWaypoints);

    targetButton = new QPushButton(this);
    targetButton->setCheckable(true);
    targetButton->setChecked(false);
    targetButton->setIcon(QIcon(QString::fromUtf8(":/images/status/weather-clear.svg")));

    QPushButton* recenterButton = new QPushButton(this);
    recenterButton->setText("Recenter Camera");

    QCheckBox* followCameraCheckBox = new QCheckBox(this);
    followCameraCheckBox->setText("Follow Camera");
    followCameraCheckBox->setChecked(followCamera);

    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(gridCheckBox, 1, 0);
    layout->addWidget(trailCheckBox, 1, 1);
    layout->addWidget(waypointsCheckBox, 1, 2);
    layout->addItem(new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 3);
    layout->addWidget(targetButton, 1, 4);
    layout->addItem(new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 5);
    layout->addWidget(recenterButton, 1, 6);
    layout->addWidget(followCameraCheckBox, 1, 7);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    setLayout(layout);

    connect(gridCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showGrid(int)));
    connect(trailCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showTrail(int)));
    connect(waypointsCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showWaypoints(int)));
    connect(recenterButton, SIGNAL(clicked()), this, SLOT(recenter()));
    connect(followCameraCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(toggleFollowCamera(int)));
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void Pixhawk3DWidget::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL && this->uas != uas)
    {
        // Disconnect any previously connected active MAV
        //disconnect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    }

    this->uas = uas;
}

void
Pixhawk3DWidget::showGrid(int32_t state)
{
    if (state == Qt::Checked)
    {
        displayGrid = true;
    }
    else
    {
        displayGrid = false;
    }
}

void
Pixhawk3DWidget::showTrail(int32_t state)
{
    if (state == Qt::Checked)
    {
        if (!displayTrail)
        {
            trail.clear();
        }

        displayTrail = true;
    }
    else
    {
        displayTrail = false;
    }
}

void
Pixhawk3DWidget::showWaypoints(int state)
{
    if (state == Qt::Checked)
    {
        displayWaypoints = true;
    }
    else
    {
        displayWaypoints = false;
    }
}

void
Pixhawk3DWidget::recenter(void)
{
    float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
    if (uas != NULL)
    {
        robotX = uas->getLocalX();
        robotY = uas->getLocalY();
        robotZ = uas->getLocalZ();
    }

    recenterCamera(robotY, robotX, -robotZ);
}

void
Pixhawk3DWidget::toggleFollowCamera(int32_t state)
{
    if (state == Qt::Checked)
    {
        followCamera = true;
    }
    else
    {
        followCamera = false;
    }
}

void
Pixhawk3DWidget::display(void)
{
    if (uas == NULL)
    {
        return;
    }

    float robotX = uas->getLocalX();
    float robotY = uas->getLocalY();
    float robotZ = uas->getLocalZ();
    float robotRoll = uas->getRoll();
    float robotPitch = uas->getPitch();
    float robotYaw = uas->getYaw();

    if (lastRobotX == 0.0f && lastRobotY == 0.0f && lastRobotZ == 0.0f)
    {
        lastRobotX = robotX;
        lastRobotY = robotY;
        lastRobotZ = robotZ;

        recenterCamera(robotY, robotX, -robotZ);

        return;
    }

    if (followCamera)
    {
        float dx = robotY - lastRobotY;
        float dy = robotX - lastRobotX;
        float dz = lastRobotZ - robotZ;

        moveCamera(dx, dy, dz);
    }

    robotPosition->setPosition(osg::Vec3(robotY, robotX, -robotZ));
    robotAttitude->setAttitude(osg::Quat(-robotYaw, osg::Vec3f(0.0f, 0.0f, 1.0f),
                                         robotPitch, osg::Vec3f(1.0f, 0.0f, 0.0f),
                                         robotRoll, osg::Vec3f(0.0f, 1.0f, 0.0f)));

    updateHUD(robotX, robotY, robotZ, robotRoll, robotPitch, robotYaw);
    updateTrail(robotX, robotY, robotZ);
    updateTarget();
    updateWaypoints();

    // set node visibility
    rollingMap->setChildValue(gridNode, displayGrid);
    rollingMap->setChildValue(trailNode, displayTrail);
    rollingMap->setChildValue(targetNode, displayTarget);
    rollingMap->setChildValue(waypointsNode, displayWaypoints);

    lastRobotX = robotX;
    lastRobotY = robotY;
    lastRobotZ = robotZ;
}

void
Pixhawk3DWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && targetButton->isChecked())
    {
        markTarget();
    }

    Q3DWidget::mousePressEvent(event);
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createGrid(void)
{
    osg::ref_ptr<osg::Geode> geode(new osg::Geode());
    osg::ref_ptr<osg::Geometry> fineGeometry(new osg::Geometry());
    osg::ref_ptr<osg::Geometry> coarseGeometry(new osg::Geometry());
    geode->addDrawable(fineGeometry);
    geode->addDrawable(coarseGeometry);

    float radius = 10.0f;
    float resolution = 0.25f;

    osg::ref_ptr<osg::Vec3Array> fineCoords(new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> coarseCoords(new osg::Vec3Array);

    // draw a 20m x 20m grid with 0.25m resolution
    for (float i = -radius; i <= radius; i += resolution)
    {
        if (fabsf(i - roundf(i)) < 0.01f)
        {
            coarseCoords->push_back(osg::Vec3(i, -radius, 0.0f));
            coarseCoords->push_back(osg::Vec3(i, radius, 0.0f));
            coarseCoords->push_back(osg::Vec3(-radius, i, 0.0f));
            coarseCoords->push_back(osg::Vec3(radius, i, 0.0f));
        }
        else
        {
            fineCoords->push_back(osg::Vec3(i, -radius, 0.0f));
            fineCoords->push_back(osg::Vec3(i, radius, 0.0f));
            fineCoords->push_back(osg::Vec3(-radius, i, 0.0f));
            fineCoords->push_back(osg::Vec3(radius, i, 0.0f));
        }
    }

    fineGeometry->setVertexArray(fineCoords);
    coarseGeometry->setVertexArray(coarseCoords);

    osg::ref_ptr<osg::Vec4Array> color(new osg::Vec4Array);
    color->push_back(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    fineGeometry->setColorArray(color);
    coarseGeometry->setColorArray(color);
    fineGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    coarseGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    fineGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,
                                                      0, fineCoords->size()));
    coarseGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0,
                                                        coarseCoords->size()));

    osg::ref_ptr<osg::StateSet> fineStateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> fineLinewidth(new osg::LineWidth());
    fineLinewidth->setWidth(0.25f);
    fineStateset->setAttributeAndModes(fineLinewidth, osg::StateAttribute::ON);
    fineStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    fineGeometry->setStateSet(fineStateset);

    osg::ref_ptr<osg::StateSet> coarseStateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> coarseLinewidth(new osg::LineWidth());
    coarseLinewidth->setWidth(2.0f);
    coarseStateset->setAttributeAndModes(coarseLinewidth, osg::StateAttribute::ON);
    coarseStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    coarseGeometry->setStateSet(coarseStateset);

    return geode;
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createTrail(void)
{
    osg::ref_ptr<osg::Geode> geode(new osg::Geode());
    trailGeometry = new osg::Geometry();
    trailGeometry->setUseDisplayList(false);
    geode->addDrawable(trailGeometry.get());

    trailVertices = new osg::Vec3Array;
    trailGeometry->setVertexArray(trailVertices);

    trailDrawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP);
    trailGeometry->addPrimitiveSet(trailDrawArrays);

    osg::ref_ptr<osg::Vec4Array> color(new osg::Vec4Array);
    color->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    trailGeometry->setColorArray(color);
    trailGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth());
    linewidth->setWidth(1.0f);
    stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    trailGeometry->setStateSet(stateset);

    return geode;
}

osg::ref_ptr<osgEarth::MapNode>
Pixhawk3DWidget::createMap(void)
{
    osg::ref_ptr<osg::Node> model = osgDB::readNodeFile("map.earth");
    osg::ref_ptr<osgEarth::MapNode> node = osgEarth::MapNode::findMapNode(model);

    return node;
}

osg::ref_ptr<osg::Node>
Pixhawk3DWidget::createTarget(void)
{
    targetPosition = new osg::PositionAttitudeTransform;

    targetNode = new osg::Geode;
    targetPosition->addChild(targetNode);

    return targetPosition;
}

osg::ref_ptr<osg::Group>
Pixhawk3DWidget::createWaypoints(void)
{
    osg::ref_ptr<osg::Group> group(new osg::Group());

    return group;
}

void
Pixhawk3DWidget::setupHUD(void)
{
    osg::ref_ptr<osg::Vec3Array> hudBackgroundVertices(new osg::Vec3Array);
    hudBackgroundVertices->push_back(osg::Vec3(0, height(), -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), height(), -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), height() - 30, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, height() - 30, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, 0, -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), 0, -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), 25, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, 25, -1));

    osg::ref_ptr<osg::DrawElementsUInt> hudTopBackgroundIndices(
            new osg::DrawElementsUInt(osg::PrimitiveSet::POLYGON, 0));
    hudTopBackgroundIndices->push_back(0);
    hudTopBackgroundIndices->push_back(1);
    hudTopBackgroundIndices->push_back(2);
    hudTopBackgroundIndices->push_back(3);

    osg::ref_ptr<osg::DrawElementsUInt> hudBottomBackgroundIndices(
            new osg::DrawElementsUInt(osg::PrimitiveSet::POLYGON, 0));
    hudBottomBackgroundIndices->push_back(4);
    hudBottomBackgroundIndices->push_back(5);
    hudBottomBackgroundIndices->push_back(6);
    hudBottomBackgroundIndices->push_back(7);

    osg::ref_ptr<osg::Vec4Array> hudColors(new osg::Vec4Array);
    hudColors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 0.2f));

    hudBackgroundGeometry = new osg::Geometry;
    hudBackgroundGeometry->addPrimitiveSet(hudTopBackgroundIndices);
    hudBackgroundGeometry->addPrimitiveSet(hudBottomBackgroundIndices);
    hudBackgroundGeometry->setVertexArray(hudBackgroundVertices);
    hudBackgroundGeometry->setColorArray(hudColors);
    hudBackgroundGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    hudGeode->addDrawable(hudBackgroundGeometry);

    statusText = new osgText::Text;
    statusText->setCharacterSize(11);
    statusText->setFont("images/Vera.ttf");
    statusText->setAxisAlignment(osgText::Text::SCREEN);
    statusText->setPosition(osg::Vec3(10, height() - 10, -1.5));
    statusText->setColor(osg::Vec4(255, 255, 255, 1));

    hudGeode->addDrawable(statusText);
}

void
Pixhawk3DWidget::updateHUD(float robotX, float robotY, float robotZ,
                         float robotRoll, float robotPitch, float robotYaw)
{
    osg::ref_ptr<osg::Vec3Array> hudBackgroundVertices(new osg::Vec3Array);
    hudBackgroundVertices->push_back(osg::Vec3(0, height(), -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), height(), -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), height() - 30, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, height() - 30, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, 0, -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), 0, -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), 25, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, 25, -1));
    hudBackgroundGeometry->setVertexArray(hudBackgroundVertices);

    statusText->setPosition(osg::Vec3(10, height() - 20, -1.5));

    std::pair<double,double> cursorPosition =
            getGlobalCursorPosition(getMouseX(), getMouseY(), -robotZ);

    std::ostringstream oss;
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss.precision(2);
    oss << " x = " << robotX <<
            " y = " << robotY <<
            " z = " << robotZ <<
            " r = " << robotRoll <<
            " p = " << robotPitch <<
            " y = " << robotYaw <<
            " Cursor [" << cursorPosition.first <<
            " " << cursorPosition.second << "]";
    statusText->setText(oss.str());
}

void
Pixhawk3DWidget::updateTrail(float robotX, float robotY, float robotZ)
{
    if (robotX == 0.0f || robotY == 0.0f || robotZ == 0.0f)
    {
        return;
    }

    bool addToTrail = false;
    if (trail.size() > 0)
    {
        if (fabsf(robotX - trail[trail.size() - 1].x()) > 0.01f ||
            fabsf(robotY - trail[trail.size() - 1].y()) > 0.01f ||
            fabsf(robotZ - trail[trail.size() - 1].z()) > 0.01f)
        {
            addToTrail = true;
        }
    }
    else
    {
        addToTrail = true;
    }

    if (addToTrail)
    {
        osg::Vec3 p(robotX, robotY, robotZ);
        if (trail.size() == trail.capacity())
        {
            memcpy(trail.data(), trail.data() + 1,
                   (trail.size() - 1) * sizeof(osg::Vec3));
            trail[trail.size() - 1] = p;
        }
        else
        {
            trail.append(p);
        }
    }

    trailVertices->clear();
    for (int i = 0; i < trail.size(); ++i)
    {
        trailVertices->push_back(osg::Vec3(trail[i].y() - robotY,
                                           trail[i].x() - robotX,
                                           -(trail[i].z() - robotZ)));
    }

    trailDrawArrays->setFirst(0);
    trailDrawArrays->setCount(trailVertices->size());
    trailGeometry->dirtyBound();
}

void
Pixhawk3DWidget::updateTarget(void)
{
    static double radius = 0.2;
    static bool expand = true;

    if (radius < 0.1)
    {
        expand = true;
    }
    else if (radius > 0.25)
    {
        expand = false;
    }

    if (targetNode->getNumDrawables() > 0)
    {
        targetNode->removeDrawables(0, targetNode->getNumDrawables());
    }

    osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable;
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere;
    sphere->setRadius(radius);
    sd->setShape(sphere);
    sd->setColor(osg::Vec4(0.0f, 0.7f, 1.0f, 1.0f));

    targetNode->addDrawable(sd);

    if (expand)
    {
        radius += 0.02;
    }
    else
    {
        radius -= 0.02;
    }
}

void
Pixhawk3DWidget::updateWaypoints(void)
{
    if (uas)
    {
        if (waypointsNode->getNumChildren() > 0)
        {
            waypointsNode->removeChild(0, waypointsNode->getNumChildren());
        }

        const QVector<Waypoint *>& list = uas->getWaypointManager().getWaypointList();

        for (int i = 0; i < list.size(); i++)
        {
            osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable;
            osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere;
            sphere->setRadius(0.2);
            sd->setShape(sphere);

            if (list.at(i)->getCurrent())
            {
                sd->setColor(osg::Vec4(1.0f, 0.3f, 0.3f, 1.0f));
            }
            else
            {
                sd->setColor(osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f));
            }

            osg::ref_ptr<osg::Geode> geode = new osg::Geode;
            geode->addDrawable(sd);

            osg::ref_ptr<osg::PositionAttitudeTransform> pat =
                    new osg::PositionAttitudeTransform;

            pat->setPosition(osg::Vec3d(list.at(i)->getY() - uas->getLocalY(),
                                        list.at(i)->getX() - uas->getLocalX(),
                                        0.0));

            waypointsNode->addChild(pat);
            pat->addChild(geode);
        }
    }
}


void
Pixhawk3DWidget::markTarget(void)
{
    float robotZ = 0.0f;
    if (uas != NULL)
    {
        robotZ = uas->getLocalZ();
    }

    std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(getMouseX(), getMouseY(), -robotZ);

    double targetX = cursorWorldCoords.first;
    double targetY = cursorWorldCoords.second;
    double targetZ = robotZ;

    targetPosition->setPosition(osg::Vec3d(targetY, targetX, -targetZ));

    displayTarget = true;

    if (uas)
    {
        uas->setTargetPosition(targetX, targetY, targetZ, 0.0f);
    }

    targetButton->setChecked(false);
}

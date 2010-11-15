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

#include <sys/time.h>
#include <sstream>

#include <osg/Geode>
#include <osg/LineWidth>
#include <osg/ShapeDrawable>

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarthDrivers/gdal/GDALOptions>

#include "PixhawkCheetahGeode.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "QGC.h"

Pixhawk3DWidget::Pixhawk3DWidget(QWidget* parent)
     : Q3DWidget(parent)
     , uas(NULL)
     , lastRedrawTime(0.0)
     , displayGrid(true)
     , displayTrail(false)
     , displayTarget(false)
     , displayWaypoints(true)
     , lockCamera(true)
{
    init(15.0f);
    setCameraParams(0.5f, 30.0f, 0.01f, 10000.0f);

    osg::Node* imagery = osgDB::readNodeFile("/home/hengli/swissimage.earth");
    root->addChild(imagery);

    // generate Pixhawk Cheetah model
    egocentricMap->addChild(PixhawkCheetahGeode::instance());

    // generate grid model
    gridNode = createGrid();
    rollingMap->addChild(gridNode);

    // generate empty trail model
    trailNode = createTrail();
    rollingMap->addChild(trailNode);

    // generate target model
    targetNode = createTarget();
    rollingMap->addChild(targetNode);

    // generate waypoint model
    waypointsNode = createWaypoints();
    rollingMap->addChild(waypointsNode);

    setupHUD();

    setDisplayFunc(display, this);
//    setMouseFunc(mouse, this);
    addTimerFunc(100, timer, this);

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

    QPushButton* recenterButton = new QPushButton(this);
    recenterButton->setText("Recenter Camera");

    QCheckBox* lockCameraCheckBox = new QCheckBox(this);
    lockCameraCheckBox->setText("Lock Camera");
    lockCameraCheckBox->setChecked(lockCamera);

    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(gridCheckBox, 1, 0);
    layout->addWidget(trailCheckBox, 1, 1);
    layout->addWidget(waypointsCheckBox, 1, 2);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 3);
    layout->addWidget(recenterButton, 1, 4);
    layout->addWidget(lockCameraCheckBox, 1, 5);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    //layout->setColumnStretch(0, 1);
    layout->setColumnStretch(2, 50);
    setLayout(layout);

    connect(gridCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showGrid(int)));
    connect(trailCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showTrail(int)));
    connect(recenterButton, SIGNAL(clicked()), this, SLOT(recenterCamera()));
    connect(lockCameraCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(toggleLockCamera(int)));
}

void
Pixhawk3DWidget::display(void* clientData)
{
    Pixhawk3DWidget* map3d = reinterpret_cast<Pixhawk3DWidget *>(clientData);
    map3d->displayHandler();
}

void
Pixhawk3DWidget::displayHandler(void)
{
    float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
    float robotRoll = 0.0f, robotPitch = 0.0f, robotYaw = 0.0f;
    if (uas != NULL)
    {
        robotX = uas->getLocalX();
        robotY = uas->getLocalY();
        robotZ = uas->getLocalZ();
        robotRoll = uas->getRoll();
        robotPitch = uas->getPitch();
        robotYaw = uas->getYaw();
    }

    robotPosition->setPosition(osg::Vec3(robotY, robotX, -robotZ));
    robotAttitude->setAttitude(osg::Quat(-robotYaw, osg::Vec3f(0.0f, 0.0f, 1.0f),
                                         robotPitch, osg::Vec3f(1.0f, 0.0f, 0.0f),
                                         robotRoll, osg::Vec3f(0.0f, 1.0f, 0.0f)));

    updateHUD(robotX, robotY, robotZ, robotRoll, robotPitch, robotYaw);
    updateTrail(robotX, robotY, robotZ);
    updateWaypoints();

    // set node visibility
    rollingMap->setChildValue(gridNode, displayGrid);
    rollingMap->setChildValue(trailNode, displayTrail);
    rollingMap->setChildValue(targetNode, displayTarget);
    rollingMap->setChildValue(waypointsNode, displayWaypoints);
}

void
Pixhawk3DWidget::mouse(Qt::MouseButton button, MouseState state,
                    int32_t x, int32_t y, void* clientData)
{
    Pixhawk3DWidget* map3d = reinterpret_cast<Pixhawk3DWidget *>(clientData);
    map3d->mouseHandler(button, state, x, y);
}

void
Pixhawk3DWidget::mouseHandler(Qt::MouseButton button, MouseState state,
                           int32_t x, int32_t y)
{
    if (button == Qt::RightButton && state == MOUSE_STATE_DOWN)
    {
        QMenu menu(this);
        QAction* targetAction = menu.addAction(tr("Mark as Target"));
        connect(targetAction, SIGNAL(triggered()), this, SLOT(markTarget()));
        menu.exec(mapToGlobal(QPoint(x, y)));
    }
}

void
Pixhawk3DWidget::timer(void* clientData)
{
    Pixhawk3DWidget* map3d = reinterpret_cast<Pixhawk3DWidget *>(clientData);
    map3d->timerHandler();
}

void
Pixhawk3DWidget::timerHandler(void)
{
    double timeLapsed = getTime() - lastRedrawTime;
    if (timeLapsed > 0.1)
    {
        forceRedraw();
        lastRedrawTime = getTime();
    }
    addTimerFunc(100, timer, this);
}

double
Pixhawk3DWidget::getTime(void) const
{
     struct timeval tv;

     gettimeofday(&tv, NULL);

     return static_cast<double>(tv.tv_sec) +
             static_cast<double>(tv.tv_usec) / 1000000.0;
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
Pixhawk3DWidget::markTarget(void)
{
    std::pair<float,float> mouseWorldCoords =
            getGlobalCursorPosition(getLastMouseX(), getLastMouseY());

    float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
    if (uas != NULL)
    {
        robotX = uas->getLocalX();
        robotY = uas->getLocalY();
        robotZ = uas->getLocalZ();
    }

    targetPosition.x() = mouseWorldCoords.first + robotX;
    targetPosition.y() = mouseWorldCoords.second + robotY;
    targetPosition.z() = robotZ;

    displayTarget = true;

    if (uas)
    {
        uas->setTargetPosition(targetPosition.x(),
                               targetPosition.y(),
                               targetPosition.z(),
                               0.0f);
    }
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
            trailVertices->clear();
        }

        displayTrail = true;
    }
    else
    {
        displayTrail = false;
    }
}

void
Pixhawk3DWidget::recenterCamera(void)
{
    recenter();
}

void
Pixhawk3DWidget::toggleLockCamera(int32_t state)
{
    if (state == Qt::Checked)
    {
        lockCamera = true;
    }
    else
    {
        lockCamera = false;
    }
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createGrid(void)
{
    osg::ref_ptr<osg::Geode> geode(new osg::Geode());
    osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry());
    geode->addDrawable(geometry.get());

    float radius = 10.0f;
    float resolution = 0.25f;

    osg::ref_ptr<osg::Vec3Array> coords(new osg::Vec3Array);

    // draw a 20m x 20m grid with 0.25m resolution
    for (float i = -radius; i <= radius; i += resolution)
    {
        coords->push_back(osg::Vec3(i, -radius, 0.0f));
        coords->push_back(osg::Vec3(i, radius, 0.0f));
        coords->push_back(osg::Vec3(-radius, i, 0.0f));
        coords->push_back(osg::Vec3(radius, i, 0.0f));
    }

    geometry->setVertexArray(coords);

    osg::ref_ptr<osg::Vec4Array> color(new osg::Vec4Array);
    color->push_back(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    geometry->setColorArray(color);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, coords->size()));

    osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth());
    linewidth->setWidth(0.25f);
    stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geometry->setStateSet(stateset);

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

osg::ref_ptr<osg::Group>
Pixhawk3DWidget::createTarget(void)
{
    osg::ref_ptr<osg::Group> geode(new osg::Group());

    return geode;
}

osg::ref_ptr<osg::Group>
Pixhawk3DWidget::createWaypoints(void)
{
    osg::ref_ptr<osg::Group> geode(new osg::Group());

    return geode;
}

void
Pixhawk3DWidget::setupHUD(void)
{
    osg::ref_ptr<osg::Vec3Array> hudBackgroundVertices(new osg::Vec3Array);
    hudBackgroundVertices->push_back(osg::Vec3(0, height(), -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), height(), -1));
    hudBackgroundVertices->push_back(osg::Vec3(width(), height() - 30, -1));
    hudBackgroundVertices->push_back(osg::Vec3(0, height() - 30, -1));

    osg::ref_ptr<osg::DrawElementsUInt> hudBackgroundIndices(
            new osg::DrawElementsUInt(osg::PrimitiveSet::POLYGON, 0));
    hudBackgroundIndices->push_back(0);
    hudBackgroundIndices->push_back(1);
    hudBackgroundIndices->push_back(2);
    hudBackgroundIndices->push_back(3);

    osg::ref_ptr<osg::Vec4Array> hudColors(new osg::Vec4Array);
    hudColors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 0.2f));

    hudBackgroundGeometry = new osg::Geometry;
    hudBackgroundGeometry->addPrimitiveSet(hudBackgroundIndices);
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
    hudBackgroundGeometry->setVertexArray(hudBackgroundVertices);

    statusText->setPosition(osg::Vec3(10, height() - 20, -1.5));

    std::pair<float,float> cursorPosition =
            getGlobalCursorPosition(getMouseX(), getMouseY());

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
Pixhawk3DWidget::updateTarget(float robotX, float robotY, float robotZ)
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

    osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable;
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere;
    sphere->setRadius(radius);
    sd->setShape(sphere);
    sd->setColor(osg::Vec4(0.0f, 0.7f, 1.0f, 1.0f));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(sd);

    osg::ref_ptr<osg::PositionAttitudeTransform> pat =
            new osg::PositionAttitudeTransform;

    pat->setPosition(osg::Vec3d(targetPosition.y() - robotY,
                                targetPosition.x() - robotX,
                                0.0));

    targetNode->addChild(pat);
    pat->addChild(geode);

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

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
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#include "Pixhawk3DWidget.h"

#include <sstream>

#include <osg/Geode>
#include <osg/Image>
#include <osgDB/ReadFile>
#include <osg/LineWidth>
#include <osg/ShapeDrawable>

#include "PixhawkCheetahGeode.h"
#include "UASManager.h"

#include "QGC.h"

#ifdef QGC_PROTOBUF_ENABLED
#include <tr1/memory>
#include <pixhawk/pixhawk.pb.h>
#endif

Pixhawk3DWidget::Pixhawk3DWidget(QWidget* parent)
    : Q3DWidget(parent)
    , uas(NULL)
    , mode(DEFAULT_MODE)
    , selectedWpIndex(-1)
    , displayGrid(true)
    , displayTrail(false)
    , displayImagery(true)
    , displayWaypoints(true)
    , displayRGBD2D(false)
    , displayRGBD3D(true)
    , displayObstacleList(true)
    , enableRGBDColor(false)
    , enableTarget(false)
    , followCamera(true)
    , frame(MAV_FRAME_LOCAL_NED)
    , lastRobotX(0.0f)
    , lastRobotY(0.0f)
    , lastRobotZ(0.0f)
{
    setCameraParams(2.0f, 30.0f, 0.01f, 10000.0f);
    init(15.0f);

    // generate Pixhawk Cheetah model
    vehicleModel = PixhawkCheetahGeode::instance();
    egocentricMap->addChild(vehicleModel);

    // generate grid model
    gridNode = createGrid();
    rollingMap->addChild(gridNode);

    // generate empty trail model
    trailNode = createTrail();
    rollingMap->addChild(trailNode);

    // generate map model
    mapNode = createMap();
    rollingMap->addChild(mapNode);

    // generate waypoint model
    waypointGroupNode = new WaypointGroupNode;
    waypointGroupNode->init();
    rollingMap->addChild(waypointGroupNode);

    // generate target model
    targetNode = createTarget();
    rollingMap->addChild(targetNode);

    // generate RGBD model
    rgbd3DNode = createRGBD3D();
    rollingMap->addChild(rgbd3DNode);

#ifdef QGC_PROTOBUF_ENABLED
    obstacleGroupNode = new ObstacleGroupNode;
    obstacleGroupNode->init();
    rollingMap->addChild(obstacleGroupNode);
#endif

    setupHUD();

    // find available vehicle models in models folder
    vehicleModels = findVehicleModels();

    buildLayout();

    updateHUD(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, "32N");

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));
}

Pixhawk3DWidget::~Pixhawk3DWidget()
{

}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void
Pixhawk3DWidget::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL && this->uas != uas)
    {
        // Disconnect any previously connected active MAV
        //disconnect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    }

    this->uas = uas;
}

void
Pixhawk3DWidget::selectFrame(QString text)
{
    if (text.compare("Global") == 0)
    {
        frame = MAV_FRAME_GLOBAL;
    }
    else if (text.compare("Local") == 0)
    {
        frame = MAV_FRAME_LOCAL_NED;
    }

    getPosition(lastRobotX, lastRobotY, lastRobotZ);

    recenter();
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
Pixhawk3DWidget::selectMapSource(int index)
{
    mapNode->setImageryType(static_cast<Imagery::ImageryType>(index));

    if (mapNode->getImageryType() == Imagery::BLANK_MAP)
    {
        displayImagery = false;
    }
    else
    {
        displayImagery = true;
    }
}

void
Pixhawk3DWidget::selectVehicleModel(int index)
{
    egocentricMap->removeChild(vehicleModel);
    vehicleModel = vehicleModels.at(index);
    egocentricMap->addChild(vehicleModel);
}

void
Pixhawk3DWidget::recenter(void)
{
    double robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
    getPosition(robotX, robotY, robotZ);

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
Pixhawk3DWidget::selectTargetHeading(void)
{
    if (!uas)
    {
        return;
    }

    osg::Vec2d p;

    if (frame == MAV_FRAME_GLOBAL)
    {
        double altitude = uas->getAltitude();

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(getMouseX(), getMouseY(), altitude);

        p.set(cursorWorldCoords.first, cursorWorldCoords.second);
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        double z = uas->getLocalZ();

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(getMouseX(), getMouseY(), -z);

        p.set(cursorWorldCoords.first, cursorWorldCoords.second);
    }

    target.z() = atan2(p.y() - target.y(), p.x() - target.x());
}

void
Pixhawk3DWidget::selectTarget(void)
{
    if (!uas)
    {
        return;
    }

    if (frame == MAV_FRAME_GLOBAL)
    {
        double altitude = uas->getAltitude();

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(cachedMousePos.x(), cachedMousePos.y(),
                                    altitude);

        target.set(cursorWorldCoords.first, cursorWorldCoords.second, 0.0);
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        double z = uas->getLocalZ();

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(cachedMousePos.x(), cachedMousePos.y(), -z);

        target.set(cursorWorldCoords.first, cursorWorldCoords.second, 0.0);
    }

    enableTarget = true;

    mode = SELECT_TARGET_HEADING_MODE;
}

void
Pixhawk3DWidget::setTarget(void)
{
    selectTargetHeading();

    uas->setTargetPosition(target.x(), target.y(), 0.0,
                           osg::RadiansToDegrees(target.z()));
}

void
Pixhawk3DWidget::insertWaypoint(void)
{
    if (!uas)
    {
        return;
    }

    Waypoint* wp = NULL;
    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude = uas->getLatitude();
        double longitude = uas->getLongitude();
        double altitude = uas->getAltitude();
        double x, y;
        QString utmZone;
        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(cachedMousePos.x(), cachedMousePos.y(),
                                    altitude);

        Imagery::UTMtoLL(cursorWorldCoords.first, cursorWorldCoords.second, utmZone,
                         latitude, longitude);

        wp = new Waypoint(0, longitude, latitude, altitude, 0.0, 0.25);
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        double z = uas->getLocalZ();

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(cachedMousePos.x(), cachedMousePos.y(), -z);

        wp = new Waypoint(0, cursorWorldCoords.first,
                          cursorWorldCoords.second, z, 0.0, 0.25);
    }

    if (wp)
    {
        wp->setFrame(frame);
        uas->getWaypointManager()->addWaypointEditable(wp);
    }

    selectedWpIndex = wp->getId();
    mode = MOVE_WAYPOINT_HEADING_MODE;
}

void
Pixhawk3DWidget::moveWaypointPosition(void)
{
    if (mode != MOVE_WAYPOINT_POSITION_MODE)
    {
        mode = MOVE_WAYPOINT_POSITION_MODE;
        return;
    }

    if (!uas)
    {
        return;
    }

    const QVector<Waypoint *> waypoints =
        uas->getWaypointManager()->getWaypointEditableList();
    Waypoint* waypoint = waypoints.at(selectedWpIndex);

    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude = uas->getLatitude();
        double longitude = uas->getLongitude();
        double altitude = uas->getAltitude();
        double x, y;
        QString utmZone;
        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(getMouseX(), getMouseY(), altitude);

        Imagery::UTMtoLL(cursorWorldCoords.first, cursorWorldCoords.second,
                         utmZone, latitude, longitude);

        waypoint->setX(longitude);
        waypoint->setY(latitude);
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        double z = uas->getLocalZ();

        std::pair<double,double> cursorWorldCoords =
            getGlobalCursorPosition(getMouseX(), getMouseY(), -z);

        waypoint->setX(cursorWorldCoords.first);
        waypoint->setY(cursorWorldCoords.second);
    }
}

void
Pixhawk3DWidget::moveWaypointHeading(void)
{
    if (mode != MOVE_WAYPOINT_HEADING_MODE)
    {
        mode = MOVE_WAYPOINT_HEADING_MODE;
        return;
    }

    if (!uas)
    {
        return;
    }

    const QVector<Waypoint *> waypoints =
        uas->getWaypointManager()->getWaypointEditableList();
    Waypoint* waypoint = waypoints.at(selectedWpIndex);

    double x = 0.0, y = 0.0, z = 0.0;

    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude = waypoint->getY();
        double longitude = waypoint->getX();
        z = -waypoint->getZ();
        QString utmZone;
        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        z = uas->getLocalZ();
    }

    std::pair<double,double> cursorWorldCoords =
        getGlobalCursorPosition(getMouseX(), getMouseY(), -z);

    double yaw = atan2(cursorWorldCoords.second - waypoint->getY(),
                       cursorWorldCoords.first - waypoint->getX());
    yaw = osg::RadiansToDegrees(yaw);

    waypoint->setYaw(yaw);
}

void
Pixhawk3DWidget::deleteWaypoint(void)
{
    if (uas)
    {
        uas->getWaypointManager()->removeWaypoint(selectedWpIndex);
    }
}

void
Pixhawk3DWidget::setWaypointAltitude(void)
{
    if (!uas)
    {
        return;
    }

    bool ok;
    const QVector<Waypoint *> waypoints =
        uas->getWaypointManager()->getWaypointEditableList();
    Waypoint* waypoint = waypoints.at(selectedWpIndex);

    double altitude = waypoint->getZ();
    if (frame == MAV_FRAME_LOCAL_NED)
    {
        altitude = -altitude;
    }

    double newAltitude =
        QInputDialog::getDouble(this, tr("Set altitude of waypoint %1").arg(selectedWpIndex),
                                tr("Altitude (m):"), waypoint->getZ(), -1000.0, 1000.0, 1, &ok);
    if (ok)
    {
        if (frame == MAV_FRAME_GLOBAL)
        {
            waypoint->setZ(newAltitude);
        }
        else if (frame == MAV_FRAME_LOCAL_NED)
        {
            waypoint->setZ(-newAltitude);
        }
    }
}

void
Pixhawk3DWidget::clearAllWaypoints(void)
{
    if (uas)
    {
        const QVector<Waypoint *> waypoints =
            uas->getWaypointManager()->getWaypointEditableList();
        for (int i = waypoints.size() - 1; i >= 0; --i)
        {
            uas->getWaypointManager()->removeWaypoint(i);
        }
    }
}

QVector< osg::ref_ptr<osg::Node> >
Pixhawk3DWidget::findVehicleModels(void)
{
    QDir directory("models");
    QStringList files = directory.entryList(QStringList("*.osg"), QDir::Files);

    QVector< osg::ref_ptr<osg::Node> > nodes;

    // add Pixhawk Bravo model
    nodes.push_back(PixhawkCheetahGeode::instance());

    // add sphere of 0.05m radius
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3f(0.0f, 0.0f, 0.0f), 0.05f);
    osg::ref_ptr<osg::ShapeDrawable> sphereDrawable = new osg::ShapeDrawable(sphere);
    sphereDrawable->setColor(osg::Vec4f(0.5f, 0.0f, 0.5f, 1.0f));
    osg::ref_ptr<osg::Geode> sphereGeode = new osg::Geode;
    sphereGeode->addDrawable(sphereDrawable);
    sphereGeode->setName("Sphere (0.1m)");
    nodes.push_back(sphereGeode);

    // add all other models in folder
    for (int i = 0; i < files.size(); ++i)
    {
        osg::ref_ptr<osg::Node> node =
            osgDB::readNodeFile(directory.absoluteFilePath(files[i]).toStdString().c_str());

        if (node)
        {
            nodes.push_back(node);
        }
        else
        {
            printf("%s\n", QString("ERROR: Could not load file " + directory.absoluteFilePath(files[i]) + "\n").toStdString().c_str());
        }
    }

//    QStringList dirs = directory.entryList(QDir::Dirs);
//    // Add models in subfolders
//    for (int i = 0; i < dirs.size(); ++i)
//    {
//        // Handling the current directory
//        QStringList currFiles = QDir(dirs[i]).entryList(QStringList("*.ac"), QDir::Files);

//        // Load the file
//        osg::ref_ptr<osg::Node> node =
//                osgDB::readNodeFile(directory.absoluteFilePath(currFiles.first()).toStdString().c_str());

//        if (node)
//        {

//        nodes.push_back(node);
//        }
//        else
//        {
//            printf(QString("ERROR: Could not load file " + directory.absoluteFilePath(files[i]) + "\n").toStdString().c_str());
//        }
//    }

    return nodes;
}

void
Pixhawk3DWidget::buildLayout(void)
{
    QComboBox* frameComboBox = new QComboBox(this);
    frameComboBox->addItem("Local");
    frameComboBox->addItem("Global");
    frameComboBox->setFixedWidth(70);

    QCheckBox* gridCheckBox = new QCheckBox(this);
    gridCheckBox->setText("Grid");
    gridCheckBox->setChecked(displayGrid);

    QCheckBox* trailCheckBox = new QCheckBox(this);
    trailCheckBox->setText("Trail");
    trailCheckBox->setChecked(displayTrail);

    QCheckBox* waypointsCheckBox = new QCheckBox(this);
    waypointsCheckBox->setText("Waypoints");
    waypointsCheckBox->setChecked(displayWaypoints);

    QLabel* mapLabel = new QLabel("Map", this);
    QComboBox* mapComboBox = new QComboBox(this);
    mapComboBox->addItem("None");
    mapComboBox->addItem("Map (Google)");
    mapComboBox->addItem("Satellite (Google)");

    QLabel* modelLabel = new QLabel("Vehicle", this);
    QComboBox* modelComboBox = new QComboBox(this);
    for (int i = 0; i < vehicleModels.size(); ++i)
    {
        modelComboBox->addItem(vehicleModels[i]->getName().c_str());
    }

    QPushButton* recenterButton = new QPushButton(this);
    recenterButton->setText("Recenter Camera");

    QCheckBox* followCameraCheckBox = new QCheckBox(this);
    followCameraCheckBox->setText("Follow Camera");
    followCameraCheckBox->setChecked(followCamera);

    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(frameComboBox, 0, 10);
    layout->addWidget(gridCheckBox, 2, 0);
    layout->addWidget(trailCheckBox, 2, 1);
    layout->addWidget(waypointsCheckBox, 2, 2);
    layout->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 3);
    layout->addWidget(mapLabel, 2, 4);
    layout->addWidget(mapComboBox, 2, 5);
    layout->addWidget(modelLabel, 2, 6);
    layout->addWidget(modelComboBox, 2, 7);
    layout->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 8);
    layout->addWidget(recenterButton, 2, 9);
    layout->addWidget(followCameraCheckBox, 2, 10);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 100);
    layout->setRowStretch(2, 1);
    setLayout(layout);

    connect(frameComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(selectFrame(QString)));
    connect(gridCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showGrid(int)));
    connect(trailCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showTrail(int)));
    connect(waypointsCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showWaypoints(int)));
    connect(mapComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(selectMapSource(int)));
    connect(modelComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(selectVehicleModel(int)));
    connect(recenterButton, SIGNAL(clicked()), this, SLOT(recenter()));
    connect(followCameraCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(toggleFollowCamera(int)));
}

void
Pixhawk3DWidget::resizeGL(int width, int height)
{
    Q3DWidget::resizeGL(width, height);

    resizeHUD();
}

void
Pixhawk3DWidget::display(void)
{
    // set node visibility
    rollingMap->setChildValue(gridNode, displayGrid);
    rollingMap->setChildValue(trailNode, displayTrail);
    rollingMap->setChildValue(mapNode, displayImagery);
    rollingMap->setChildValue(waypointGroupNode, displayWaypoints);
    rollingMap->setChildValue(targetNode, enableTarget);
#ifdef QGC_PROTOBUF_ENABLED
    rollingMap->setChildValue(obstacleGroupNode, displayObstacleList);
#endif
    rollingMap->setChildValue(rgbd3DNode, displayRGBD3D);
    hudGroup->setChildValue(rgb2DGeode, displayRGBD2D);
    hudGroup->setChildValue(depth2DGeode, displayRGBD2D);

    if (!uas)
    {
        return;
    }

    double robotX, robotY, robotZ, robotRoll, robotPitch, robotYaw;
    QString utmZone;
    getPose(robotX, robotY, robotZ, robotRoll, robotPitch, robotYaw, utmZone);

    if (lastRobotX == 0.0f && lastRobotY == 0.0f && lastRobotZ == 0.0f)
    {
        lastRobotX = robotX;
        lastRobotY = robotY;
        lastRobotZ = robotZ;

        recenterCamera(robotY, robotX, -robotZ);
    }

    if (followCamera)
    {
        double dx = robotY - lastRobotY;
        double dy = robotX - lastRobotX;
        double dz = lastRobotZ - robotZ;

        moveCamera(dx, dy, dz);
    }

    robotPosition->setPosition(osg::Vec3d(robotY, robotX, -robotZ));
    robotAttitude->setAttitude(osg::Quat(-robotYaw, osg::Vec3d(0.0f, 0.0f, 1.0f),
                                         robotPitch, osg::Vec3d(1.0f, 0.0f, 0.0f),
                                         robotRoll, osg::Vec3d(0.0f, 1.0f, 0.0f)));

    if (displayTrail)
    {
        updateTrail(robotX, robotY, robotZ);
    }

    if (frame == MAV_FRAME_GLOBAL && displayImagery)
    {
        updateImagery(robotX, robotY, robotZ, utmZone);
    }

    if (displayWaypoints)
    {
        updateWaypoints();
    }

    if (enableTarget)
    {
        updateTarget(robotX, robotY);
    }

#ifdef QGC_PROTOBUF_ENABLED
    if (displayRGBD2D || displayRGBD3D)
    {
        updateRGBD(robotX, robotY, robotZ);
    }

    if (displayObstacleList)
    {
        updateObstacles();
    }
#endif

    updateHUD(robotX, robotY, robotZ, robotRoll, robotPitch, robotYaw, utmZone);

    lastRobotX = robotX;
    lastRobotY = robotY;
    lastRobotZ = robotZ;

    layout()->update();
}

void
Pixhawk3DWidget::keyPressEvent(QKeyEvent* event)
{
    if (!event->text().isEmpty())
    {
        switch (*(event->text().toAscii().data()))
        {
        case '1':
            displayRGBD2D = !displayRGBD2D;
            break;
        case '2':
            displayRGBD3D = !displayRGBD3D;
            break;
        case 'c':
        case 'C':
            enableRGBDColor = !enableRGBDColor;
            break;
        case 'o':
        case 'O':
            displayObstacleList = !displayObstacleList;
            break;
        }
    }

    Q3DWidget::keyPressEvent(event);
}

void
Pixhawk3DWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (mode == SELECT_TARGET_HEADING_MODE)
        {
            setTarget();
        }

        if (mode != DEFAULT_MODE)
        {
            mode = DEFAULT_MODE;
        }

        if (event->modifiers() == Qt::ShiftModifier)
        {
            selectedWpIndex = findWaypoint(event->pos());
            if (selectedWpIndex == -1)
            {
                cachedMousePos = event->pos();

                showInsertWaypointMenu(event->globalPos());
            }
            else
            {
                showEditWaypointMenu(event->globalPos());
            }

            return;
        }
    }

    Q3DWidget::mousePressEvent(event);
}

void
Pixhawk3DWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mode == SELECT_TARGET_HEADING_MODE)
    {
        selectTargetHeading();
    }
    if (mode == MOVE_WAYPOINT_POSITION_MODE)
    {
        moveWaypointPosition();
    }
    if (mode == MOVE_WAYPOINT_HEADING_MODE)
    {
        moveWaypointHeading();
    }

    Q3DWidget::mouseMoveEvent(event);
}

void
Pixhawk3DWidget::getPose(double& x, double& y, double& z,
                         double& roll, double& pitch, double& yaw,
                         QString& utmZone)
{
    if (!uas)
    {
        return;
    }

    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude = uas->getLatitude();
        double longitude = uas->getLongitude();
        double altitude = uas->getAltitude();

        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);
        z = -altitude;
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        x = uas->getLocalX();
        y = uas->getLocalY();
        z = uas->getLocalZ();
    }

    roll = uas->getRoll();
    pitch = uas->getPitch();
    yaw = uas->getYaw();
}

void
Pixhawk3DWidget::getPose(double& x, double& y, double& z,
                         double& roll, double& pitch, double& yaw)
{
    QString utmZone;
    getPose(x, y, z, roll, pitch, yaw);
}

void
Pixhawk3DWidget::getPosition(double& x, double& y, double& z,
                             QString& utmZone)
{
    if (!uas)
    {
        return;
    }

    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude = uas->getLatitude();
        double longitude = uas->getLongitude();
        double altitude = uas->getAltitude();

        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);
        z = -altitude;
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        x = uas->getLocalX();
        y = uas->getLocalY();
        z = uas->getLocalZ();
    }
}

void
Pixhawk3DWidget::getPosition(double& x, double& y, double& z)
{
    QString utmZone;
    getPosition(x, y, z, utmZone);
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
        if (fabs(i - floor(i + 0.5f)) < 0.01f)
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

    trailVertices = new osg::Vec3dArray;
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

osg::ref_ptr<Imagery>
Pixhawk3DWidget::createMap(void)
{
    return osg::ref_ptr<Imagery>(new Imagery());
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createRGBD3D(void)
{
    int frameSize = 752 * 480;

    osg::ref_ptr<osg::Geode> geode(new osg::Geode);
    osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry);

    osg::ref_ptr<osg::Vec3Array> vertices(new osg::Vec3Array(frameSize));
    geometry->setVertexArray(vertices);

    osg::ref_ptr<osg::Vec4Array> colors(new osg::Vec4Array(frameSize));
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setUseDisplayList(false);

    geode->addDrawable(geometry);

    return geode;
}

osg::ref_ptr<osg::Node>
Pixhawk3DWidget::createTarget(void)
{
    osg::ref_ptr<osg::PositionAttitudeTransform> pat =
        new osg::PositionAttitudeTransform;

    pat->setPosition(osg::Vec3d(0.0, 0.0, 0.0));

    osg::ref_ptr<osg::Cone> cone = new osg::Cone(osg::Vec3f(0.0f, 0.0f, 0.0f), 0.2f, 0.6f);
    osg::ref_ptr<osg::ShapeDrawable> coneDrawable = new osg::ShapeDrawable(cone);
    coneDrawable->setColor(osg::Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
    coneDrawable->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    osg::ref_ptr<osg::Geode> coneGeode = new osg::Geode;
    coneGeode->addDrawable(coneDrawable);
    coneGeode->setName("Target");

    pat->addChild(coneGeode);

    return pat;
}

void
Pixhawk3DWidget::setupHUD(void)
{
    osg::ref_ptr<osg::Vec4Array> hudColors(new osg::Vec4Array);
    hudColors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 0.5f));
    hudColors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

    hudBackgroundGeometry = new osg::Geometry;
    hudBackgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                           0, 4));
    hudBackgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                           4, 4));
    hudBackgroundGeometry->setColorArray(hudColors);
    hudBackgroundGeometry->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    hudBackgroundGeometry->setUseDisplayList(false);

    statusText = new osgText::Text;
    statusText->setCharacterSize(11);
    statusText->setFont(font);
    statusText->setAxisAlignment(osgText::Text::SCREEN);
    statusText->setColor(osg::Vec4(255, 255, 255, 1));

    osg::ref_ptr<osg::Geode> statusGeode = new osg::Geode;
    statusGeode->addDrawable(hudBackgroundGeometry);
    statusGeode->addDrawable(statusText);
    hudGroup->addChild(statusGeode);

    rgbImage = new osg::Image;
    rgb2DGeode = new ImageWindowGeode;
    rgb2DGeode->init("RGB Image", osg::Vec4(0.0f, 0.0f, 0.1f, 1.0f),
                     rgbImage, font);
    hudGroup->addChild(rgb2DGeode);

    depthImage = new osg::Image;
    depth2DGeode = new ImageWindowGeode;
    depth2DGeode->init("Depth Image", osg::Vec4(0.0f, 0.0f, 0.1f, 1.0f),
                       depthImage, font);
    hudGroup->addChild(depth2DGeode);

    scaleGeode = new HUDScaleGeode;
    scaleGeode->init(font);
    hudGroup->addChild(scaleGeode);
}

void
Pixhawk3DWidget::resizeHUD(void)
{
    int topHUDHeight = 25;
    int bottomHUDHeight = 25;

    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(hudBackgroundGeometry->getVertexArray());
    if (vertices == NULL || vertices->size() != 8)
    {
        osg::ref_ptr<osg::Vec3Array> newVertices = new osg::Vec3Array(8);
        hudBackgroundGeometry->setVertexArray(newVertices);

        vertices = static_cast<osg::Vec3Array*>(hudBackgroundGeometry->getVertexArray());
    }

    (*vertices)[0] = osg::Vec3(0, height(), -1);
    (*vertices)[1] = osg::Vec3(width(), height(), -1);
    (*vertices)[2] = osg::Vec3(width(), height() - topHUDHeight, -1);
    (*vertices)[3] = osg::Vec3(0, height() - topHUDHeight, -1);
    (*vertices)[4] = osg::Vec3(0, 0, -1);
    (*vertices)[5] = osg::Vec3(width(), 0, -1);
    (*vertices)[6] = osg::Vec3(width(), bottomHUDHeight, -1);
    (*vertices)[7] = osg::Vec3(0, bottomHUDHeight, -1);

    statusText->setPosition(osg::Vec3(10, height() - 15, -1.5));

    if (rgb2DGeode.valid() && depth2DGeode.valid())
    {
        int windowWidth = (width() - 20) / 2;
        int windowHeight = 3 * windowWidth / 4;
        rgb2DGeode->setAttributes(10, (height() - windowHeight) / 2,
                                  windowWidth, windowHeight);
        depth2DGeode->setAttributes(width() / 2, (height() - windowHeight) / 2,
                                    windowWidth, windowHeight);
    }
}

void
Pixhawk3DWidget::updateHUD(double robotX, double robotY, double robotZ,
                           double robotRoll, double robotPitch, double robotYaw,
                           const QString& utmZone)
{
    std::pair<double,double> cursorPosition =
        getGlobalCursorPosition(getMouseX(), getMouseY(), -robotZ);

    std::ostringstream oss;
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss.precision(2);
    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude, longitude;
        Imagery::UTMtoLL(robotX, robotY, utmZone, latitude, longitude);

        double cursorLatitude, cursorLongitude;
        Imagery::UTMtoLL(cursorPosition.first, cursorPosition.second,
                         utmZone, cursorLatitude, cursorLongitude);

        oss.precision(6);
        oss << " Lat = " << latitude <<
            " Lon = " << longitude;

        oss.precision(2);
        oss << " Altitude = " << -robotZ <<
            " r = " << robotRoll <<
            " p = " << robotPitch <<
            " y = " << robotYaw;

        oss.precision(6);
        oss << " Cursor [" << cursorLatitude <<
            " " << cursorLongitude << "]";
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        oss << " x = " << robotX <<
            " y = " << robotY <<
            " z = " << robotZ <<
            " r = " << robotRoll <<
            " p = " << robotPitch <<
            " y = " << robotYaw <<
            " Cursor [" << cursorPosition.first <<
            " " << cursorPosition.second << "]";
    }

    statusText->setText(oss.str());

    bool darkBackground = true;
    if (mapNode->getImageryType() == Imagery::GOOGLE_MAP)
    {
        darkBackground = false;
    }

    scaleGeode->update(height(), cameraParams.cameraFov,
                       cameraManipulator->getDistance(), darkBackground);
}

void
Pixhawk3DWidget::updateTrail(double robotX, double robotY, double robotZ)
{
    if (robotX == 0.0f || robotY == 0.0f || robotZ == 0.0f)
    {
        return;
    }

    bool addToTrail = false;
    if (trail.size() > 0)
    {
        if (fabs(robotX - trail[trail.size() - 1].x()) > 0.01f ||
                fabs(robotY - trail[trail.size() - 1].y()) > 0.01f ||
                fabs(robotZ - trail[trail.size() - 1].z()) > 0.01f)
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
        osg::Vec3d p(robotX, robotY, robotZ);
        if (trail.size() == trail.capacity())
        {
            memcpy(trail.data(), trail.data() + 1,
                   (trail.size() - 1) * sizeof(osg::Vec3d));
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
        trailVertices->push_back(osg::Vec3d(trail[i].y() - robotY,
                                            trail[i].x() - robotX,
                                            -(trail[i].z() - robotZ)));
    }

    trailDrawArrays->setFirst(0);
    trailDrawArrays->setCount(trailVertices->size());
    trailGeometry->dirtyBound();
}

void
Pixhawk3DWidget::updateImagery(double originX, double originY, double originZ,
                               const QString& zone)
{
    if (mapNode->getImageryType() == Imagery::BLANK_MAP)
    {
        return;
    }

    double viewingRadius = cameraManipulator->getDistance() * 10.0;
    if (viewingRadius < 100.0)
    {
        viewingRadius = 100.0;
    }

    double minResolution = 0.25;
    double centerResolution = cameraManipulator->getDistance() / 50.0;
    double maxResolution = 1048576.0;

    Imagery::ImageryType imageryType = mapNode->getImageryType();
    switch (imageryType)
    {
    case Imagery::GOOGLE_MAP:
        minResolution = 0.25;
        break;
    case Imagery::GOOGLE_SATELLITE:
        minResolution = 0.5;
        break;
    case Imagery::SWISSTOPO_SATELLITE:
        minResolution = 0.25;
        maxResolution = 0.25;
        break;
    default:
        {}
    }

    double resolution = minResolution;
    while (resolution * 2.0 < centerResolution)
    {
        resolution *= 2.0;
    }

    if (resolution > maxResolution)
    {
        resolution = maxResolution;
    }

    mapNode->draw3D(viewingRadius,
                    resolution,
                    cameraManipulator->getCenter().y(),
                    cameraManipulator->getCenter().x(),
                    originX,
                    originY,
                    originZ,
                    zone);

    // prefetch map tiles
    if (resolution / 2.0 >= minResolution)
    {
        mapNode->prefetch3D(viewingRadius / 2.0,
                            resolution / 2.0,
                            cameraManipulator->getCenter().y(),
                            cameraManipulator->getCenter().x(),
                            zone);
    }
    if (resolution * 2.0 <= maxResolution)
    {
        mapNode->prefetch3D(viewingRadius * 2.0,
                            resolution * 2.0,
                            cameraManipulator->getCenter().y(),
                            cameraManipulator->getCenter().x(),
                            zone);
    }

    mapNode->update();
}

void
Pixhawk3DWidget::updateWaypoints(void)
{
    waypointGroupNode->update(frame, uas);
}

void
Pixhawk3DWidget::updateTarget(double robotX, double robotY)
{
    osg::PositionAttitudeTransform* pat =
        dynamic_cast<osg::PositionAttitudeTransform*>(targetNode.get());

    pat->setPosition(osg::Vec3d(target.y() - robotY, target.x() - robotX, 0.0));
    pat->setAttitude(osg::Quat(target.z() - M_PI_2, osg::Vec3d(1.0f, 0.0f, 0.0f),
                               M_PI_2, osg::Vec3d(0.0f, 1.0f, 0.0f),
                               0.0, osg::Vec3d(0.0f, 0.0f, 1.0f)));

    osg::Geode* geode = dynamic_cast<osg::Geode*>(pat->getChild(0));
    osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>(geode->getDrawable(0));


    sd->setColor(osg::Vec4f(1.0f, 0.8f, 0.0f, 1.0f));
}

float colormap_jet[128][3] = {
    {0.0f,0.0f,0.53125f},
    {0.0f,0.0f,0.5625f},
    {0.0f,0.0f,0.59375f},
    {0.0f,0.0f,0.625f},
    {0.0f,0.0f,0.65625f},
    {0.0f,0.0f,0.6875f},
    {0.0f,0.0f,0.71875f},
    {0.0f,0.0f,0.75f},
    {0.0f,0.0f,0.78125f},
    {0.0f,0.0f,0.8125f},
    {0.0f,0.0f,0.84375f},
    {0.0f,0.0f,0.875f},
    {0.0f,0.0f,0.90625f},
    {0.0f,0.0f,0.9375f},
    {0.0f,0.0f,0.96875f},
    {0.0f,0.0f,1.0f},
    {0.0f,0.03125f,1.0f},
    {0.0f,0.0625f,1.0f},
    {0.0f,0.09375f,1.0f},
    {0.0f,0.125f,1.0f},
    {0.0f,0.15625f,1.0f},
    {0.0f,0.1875f,1.0f},
    {0.0f,0.21875f,1.0f},
    {0.0f,0.25f,1.0f},
    {0.0f,0.28125f,1.0f},
    {0.0f,0.3125f,1.0f},
    {0.0f,0.34375f,1.0f},
    {0.0f,0.375f,1.0f},
    {0.0f,0.40625f,1.0f},
    {0.0f,0.4375f,1.0f},
    {0.0f,0.46875f,1.0f},
    {0.0f,0.5f,1.0f},
    {0.0f,0.53125f,1.0f},
    {0.0f,0.5625f,1.0f},
    {0.0f,0.59375f,1.0f},
    {0.0f,0.625f,1.0f},
    {0.0f,0.65625f,1.0f},
    {0.0f,0.6875f,1.0f},
    {0.0f,0.71875f,1.0f},
    {0.0f,0.75f,1.0f},
    {0.0f,0.78125f,1.0f},
    {0.0f,0.8125f,1.0f},
    {0.0f,0.84375f,1.0f},
    {0.0f,0.875f,1.0f},
    {0.0f,0.90625f,1.0f},
    {0.0f,0.9375f,1.0f},
    {0.0f,0.96875f,1.0f},
    {0.0f,1.0f,1.0f},
    {0.03125f,1.0f,0.96875f},
    {0.0625f,1.0f,0.9375f},
    {0.09375f,1.0f,0.90625f},
    {0.125f,1.0f,0.875f},
    {0.15625f,1.0f,0.84375f},
    {0.1875f,1.0f,0.8125f},
    {0.21875f,1.0f,0.78125f},
    {0.25f,1.0f,0.75f},
    {0.28125f,1.0f,0.71875f},
    {0.3125f,1.0f,0.6875f},
    {0.34375f,1.0f,0.65625f},
    {0.375f,1.0f,0.625f},
    {0.40625f,1.0f,0.59375f},
    {0.4375f,1.0f,0.5625f},
    {0.46875f,1.0f,0.53125f},
    {0.5f,1.0f,0.5f},
    {0.53125f,1.0f,0.46875f},
    {0.5625f,1.0f,0.4375f},
    {0.59375f,1.0f,0.40625f},
    {0.625f,1.0f,0.375f},
    {0.65625f,1.0f,0.34375f},
    {0.6875f,1.0f,0.3125f},
    {0.71875f,1.0f,0.28125f},
    {0.75f,1.0f,0.25f},
    {0.78125f,1.0f,0.21875f},
    {0.8125f,1.0f,0.1875f},
    {0.84375f,1.0f,0.15625f},
    {0.875f,1.0f,0.125f},
    {0.90625f,1.0f,0.09375f},
    {0.9375f,1.0f,0.0625f},
    {0.96875f,1.0f,0.03125f},
    {1.0f,1.0f,0.0f},
    {1.0f,0.96875f,0.0f},
    {1.0f,0.9375f,0.0f},
    {1.0f,0.90625f,0.0f},
    {1.0f,0.875f,0.0f},
    {1.0f,0.84375f,0.0f},
    {1.0f,0.8125f,0.0f},
    {1.0f,0.78125f,0.0f},
    {1.0f,0.75f,0.0f},
    {1.0f,0.71875f,0.0f},
    {1.0f,0.6875f,0.0f},
    {1.0f,0.65625f,0.0f},
    {1.0f,0.625f,0.0f},
    {1.0f,0.59375f,0.0f},
    {1.0f,0.5625f,0.0f},
    {1.0f,0.53125f,0.0f},
    {1.0f,0.5f,0.0f},
    {1.0f,0.46875f,0.0f},
    {1.0f,0.4375f,0.0f},
    {1.0f,0.40625f,0.0f},
    {1.0f,0.375f,0.0f},
    {1.0f,0.34375f,0.0f},
    {1.0f,0.3125f,0.0f},
    {1.0f,0.28125f,0.0f},
    {1.0f,0.25f,0.0f},
    {1.0f,0.21875f,0.0f},
    {1.0f,0.1875f,0.0f},
    {1.0f,0.15625f,0.0f},
    {1.0f,0.125f,0.0f},
    {1.0f,0.09375f,0.0f},
    {1.0f,0.0625f,0.0f},
    {1.0f,0.03125f,0.0f},
    {1.0f,0.0f,0.0f},
    {0.96875f,0.0f,0.0f},
    {0.9375f,0.0f,0.0f},
    {0.90625f,0.0f,0.0f},
    {0.875f,0.0f,0.0f},
    {0.84375f,0.0f,0.0f},
    {0.8125f,0.0f,0.0f},
    {0.78125f,0.0f,0.0f},
    {0.75f,0.0f,0.0f},
    {0.71875f,0.0f,0.0f},
    {0.6875f,0.0f,0.0f},
    {0.65625f,0.0f,0.0f},
    {0.625f,0.0f,0.0f},
    {0.59375f,0.0f,0.0f},
    {0.5625f,0.0f,0.0f},
    {0.53125f,0.0f,0.0f},
    {0.5f,0.0f,0.0f}
};

#ifdef QGC_PROTOBUF_ENABLED
void
Pixhawk3DWidget::updateRGBD(double robotX, double robotY, double robotZ)
{
    px::RGBDImage rgbdImage = uas->getRGBDImage();
    px::PointCloudXYZRGB pointCloud = uas->getPointCloud();

    if (rgbdImage.rows() > 0 && rgbdImage.cols() > 0)
    {
        rgbImage->setImage(rgbdImage.cols(), rgbdImage.rows(), 1,
                           GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                           reinterpret_cast<unsigned char *>(&(*(rgbdImage.mutable_imagedata1()))[0]),
                           osg::Image::NO_DELETE);
        rgbImage->dirty();

        QByteArray coloredDepth(rgbdImage.cols() * rgbdImage.rows() * 3, 0);
        for (uint32_t r = 0; r < rgbdImage.rows(); ++r)
        {
            const float* depth = reinterpret_cast<const float*>(rgbdImage.imagedata2().c_str() + r * rgbdImage.step2());
            uint8_t* pixel = reinterpret_cast<uint8_t*>(coloredDepth.data()) + r * rgbdImage.cols() * 3;
            for (uint32_t c = 0; c < rgbdImage.cols(); ++c)
            {
                if (depth[c] != 0)
                {
                    int idx = fminf(depth[c], 7.0f) / 7.0f * 127.0f;
                    idx = 127 - idx;

                    pixel[0] = colormap_jet[idx][2] * 255.0f;
                    pixel[1] = colormap_jet[idx][1] * 255.0f;
                    pixel[2] = colormap_jet[idx][0] * 255.0f;
                }

                pixel += 3;
            }
        }

        depthImage->setImage(rgbdImage.cols(), rgbdImage.rows(), 1,
                             GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
                             reinterpret_cast<unsigned char *>(coloredDepth.data()),
                             osg::Image::NO_DELETE);
        depthImage->dirty();
    }

    osg::Geometry* geometry = rgbd3DNode->getDrawable(0)->asGeometry();

    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(geometry->getVertexArray());
    osg::Vec4Array* colors = static_cast<osg::Vec4Array*>(geometry->getColorArray());
    for (int i = 0; i < pointCloud.points_size(); ++i)
    {
        const px::PointCloudXYZRGB_PointXYZRGB& p = pointCloud.points(i);

        double x = p.x() - robotX;
        double y = p.y() - robotY;
        double z = p.z() - robotZ;


        (*vertices)[i].set(y, x, -z);

        if (enableRGBDColor)
        {
            float rgb = p.rgb();

            float b = *(reinterpret_cast<unsigned char*>(&rgb)) / 255.0f;
            float g = *(1 + reinterpret_cast<unsigned char*>(&rgb)) / 255.0f;
            float r = *(2 + reinterpret_cast<unsigned char*>(&rgb)) / 255.0f;

            (*colors)[i].set(r, g, b, 1.0f);
        }
        else
        {
            double dist = sqrt(x * x + y * y + z * z);
            int colorIndex = static_cast<int>(fmin(dist / 7.0 * 127.0, 127.0));
            (*colors)[i].set(colormap_jet[colorIndex][0],
                             colormap_jet[colorIndex][1],
                             colormap_jet[colorIndex][2],
                             1.0f);
        }
    }

    if (geometry->getNumPrimitiveSets() == 0)
    {
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS,
                                  0, pointCloud.points_size()));
    }
    else
    {
        osg::DrawArrays* drawarrays = static_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0));
        drawarrays->setCount(pointCloud.points_size());
    }
}

void
Pixhawk3DWidget::updateObstacles(void)
{
    obstacleGroupNode->update(frame, uas);
}

#endif

int
Pixhawk3DWidget::findWaypoint(const QPoint& mousePos)
{
    if (getSceneData())
    {
        osgUtil::LineSegmentIntersector::Intersections intersections;

        if (computeIntersections(mousePos.x(), height() - mousePos.y(),
                                 intersections))
        {
            for (osgUtil::LineSegmentIntersector::Intersections::iterator
                    it = intersections.begin(); it != intersections.end(); it++)
            {
                for (uint i = 0 ; i < it->nodePath.size(); ++i)
                {
                    std::string nodeName = it->nodePath[i]->getName();
                    if (nodeName.substr(0, 2).compare("wp") == 0)
                    {
                        return atoi(nodeName.substr(2).c_str());
                    }
                }
            }
        }
    }

    return -1;
}

bool
Pixhawk3DWidget::findTarget(int mouseX, int mouseY)
{
    if (getSceneData())
    {
        osgUtil::LineSegmentIntersector::Intersections intersections;

        if (computeIntersections(mouseX, height() - mouseY, intersections))
        {
            for (osgUtil::LineSegmentIntersector::Intersections::iterator
                    it = intersections.begin(); it != intersections.end(); it++)
            {
                for (uint i = 0 ; i < it->nodePath.size(); ++i)
                {
                    std::string nodeName = it->nodePath[i]->getName();
                    if (nodeName.compare("Target") == 0)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void
Pixhawk3DWidget::showInsertWaypointMenu(const QPoint &cursorPos)
{
    QMenu menu;
    menu.addAction("Insert new waypoint", this, SLOT(insertWaypoint()));
    menu.addAction("Clear all waypoints", this, SLOT(clearAllWaypoints()));
    menu.addAction("Select target", this, SLOT(selectTarget()));
    menu.exec(cursorPos);
}

void
Pixhawk3DWidget::showEditWaypointMenu(const QPoint &cursorPos)
{
    QMenu menu;

    QString text;
    text = QString("Move waypoint %1").arg(QString::number(selectedWpIndex));
    menu.addAction(text, this, SLOT(moveWaypointPosition()));

    text = QString("Change heading of waypoint %1").arg(QString::number(selectedWpIndex));
    menu.addAction(text, this, SLOT(moveWaypointHeading()));

    text = QString("Change altitude of waypoint %1").arg(QString::number(selectedWpIndex));
    menu.addAction(text, this, SLOT(setWaypointAltitude()));

    text = QString("Delete waypoint %1").arg(QString::number(selectedWpIndex));
    menu.addAction(text, this, SLOT(deleteWaypoint()));

    menu.addAction("Clear all waypoints", this, SLOT(clearAllWaypoints()));
    menu.exec(cursorPos);
}

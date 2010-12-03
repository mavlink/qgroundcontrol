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
#include <osg/Image>
#include <osgDB/ReadFile>
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
     , displayImagery(true)
     , displayTarget(false)
     , displayWaypoints(true)
     , displayRGBD2D(false)
     , displayRGBD3D(false)
     , enableRGBDColor(true)
     , followCamera(true)
     , enableFreenect(false)
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
    allocentricMap->addChild(mapNode);

    // generate target model
    allocentricMap->addChild(createTarget());

    // generate waypoint model
    waypointsNode = createWaypoints();
    rollingMap->addChild(waypointsNode);

#ifdef QGC_LIBFREENECT_ENABLED
    freenect.reset(new Freenect());
    enableFreenect = freenect->init();
#endif

    // generate RGBD model
    if (enableFreenect)
    {
        rgbd3DNode = createRGBD3D();
        egocentricMap->addChild(rgbd3DNode);
    }

    setupHUD();

    // find available vehicle models in models folder
    vehicleModels = findVehicleModels();

    buildLayout();

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

QVector< osg::ref_ptr<osg::Node> >
Pixhawk3DWidget::findVehicleModels(void)
{
    QDir directory("models");
    QStringList files = directory.entryList(QStringList("*.osg"), QDir::Files);

    QVector< osg::ref_ptr<osg::Node> > nodes;

    // add Pixhawk Bravo model
    nodes.push_back(PixhawkCheetahGeode::instance());

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

    QLabel* modelLabel = new QLabel("Vehicle Model", this);
    QComboBox* modelComboBox = new QComboBox(this);
    for (int i = 0; i < vehicleModels.size(); ++i)
    {
        modelComboBox->addItem(vehicleModels[i]->getName().c_str());
    }

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
    layout->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 3);
    layout->addWidget(mapLabel, 1, 4);
    layout->addWidget(mapComboBox, 1, 5);
    layout->addWidget(modelLabel, 1, 6);
    layout->addWidget(modelComboBox, 1, 7);
    layout->addWidget(targetButton, 1, 8);
    layout->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 9);
    layout->addWidget(recenterButton, 1, 10);
    layout->addWidget(followCameraCheckBox, 1, 11);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    setLayout(layout);

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
Pixhawk3DWidget::display(void)
{
    if (uas == NULL)
    {
        return;
    }

    double robotX = uas->getLocalX();
    double robotY = uas->getLocalY();
    double robotZ = uas->getLocalZ();
    double robotRoll = uas->getRoll();
    double robotPitch = uas->getPitch();
    double robotYaw = uas->getYaw();

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

    if (displayImagery)
    {
        updateImagery();
    }

    if (displayTarget)
    {
        updateTarget();
    }

    if (displayWaypoints)
    {
        updateWaypoints();
    }

#ifdef QGC_LIBFREENECT_ENABLED
    if (enableFreenect && (displayRGBD2D || displayRGBD3D))
    {
        updateRGBD();
    }
#endif
    updateHUD(robotX, robotY, robotZ, robotRoll, robotPitch, robotYaw);

    // set node visibility

    rollingMap->setChildValue(gridNode, displayGrid);
    rollingMap->setChildValue(trailNode, displayTrail);
    allocentricMap->setChildValue(mapNode, displayImagery);
    rollingMap->setChildValue(targetNode, displayTarget);
    rollingMap->setChildValue(waypointsNode, displayWaypoints);
    if (enableFreenect)
    {
        egocentricMap->setChildValue(rgbd3DNode, displayRGBD3D);
    }
    hudGroup->setChildValue(rgb2DGeode, displayRGBD2D);
    hudGroup->setChildValue(depth2DGeode, displayRGBD2D);

    lastRobotX = robotX;
    lastRobotY = robotY;
    lastRobotZ = robotZ;
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
        case 'c': case 'C':
            enableRGBDColor = !enableRGBDColor;
            break;
        }
    }

    Q3DWidget::keyPressEvent(event);
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
        if (fabsf(i - floor(i + 0.5f)) < 0.01f)
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

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createRGBD3D(void)
{
    int frameSize = 640 * 480;

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

void
Pixhawk3DWidget::setupHUD(void)
{
    osg::ref_ptr<osg::Vec4Array> hudColors(new osg::Vec4Array);
    hudColors->push_back(osg::Vec4(0.0f, 0.0f, 0.0f, 0.2f));

    hudBackgroundGeometry = new osg::Geometry;
    hudBackgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                                               0, 4));
    hudBackgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                                               4, 4));
    hudBackgroundGeometry->setColorArray(hudColors);
    hudBackgroundGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    hudBackgroundGeometry->setUseDisplayList(false);

    statusText = new osgText::Text;
    statusText->setCharacterSize(11);
    statusText->setFont("images/Vera.ttf");
    statusText->setAxisAlignment(osgText::Text::SCREEN);
    statusText->setColor(osg::Vec4(255, 255, 255, 1));

    resizeHUD();

    osg::ref_ptr<osg::Geode> statusGeode = new osg::Geode;
    statusGeode->addDrawable(hudBackgroundGeometry);
    statusGeode->addDrawable(statusText);
    hudGroup->addChild(statusGeode);

    rgbImage = new osg::Image;
    rgb2DGeode = new ImageWindowGeode("RGB Image",
                                      osg::Vec4(0.0f, 0.0f, 0.1f, 1.0f),
                                      rgbImage);
    hudGroup->addChild(rgb2DGeode);

    depthImage = new osg::Image;
    depth2DGeode = new ImageWindowGeode("Depth Image",
                                        osg::Vec4(0.0f, 0.0f, 0.1f, 1.0f),
                                        depthImage);
    hudGroup->addChild(depth2DGeode);
}

void
Pixhawk3DWidget::resizeHUD(void)
{
    int topHUDHeight = 30;
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

    statusText->setPosition(osg::Vec3(10, height() - 20, -1.5));

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
                           double robotRoll, double robotPitch, double robotYaw)
{
    resizeHUD();

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

    if (!rgb.isNull())
    {
        rgbImage->setImage(640, 480, 1,
                           GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
                           reinterpret_cast<unsigned char *>(rgb->data()),
                           osg::Image::NO_DELETE);
        rgbImage->dirty();

        depthImage->setImage(640, 480, 1,
                             GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
                             reinterpret_cast<unsigned char *>(coloredDepth->data()),
                             osg::Image::NO_DELETE);
        depthImage->dirty();
    }
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
Pixhawk3DWidget::updateImagery(void)
{
    if (mapNode->getImageryType() == Imagery::BLANK_MAP)
    {
        return;
    }

    char zone[5] = "32T";

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
    default: {}
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

float colormap_jet[128][3] =
{
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

#ifdef QGC_LIBFREENECT_ENABLED
void
Pixhawk3DWidget::updateRGBD(void)
{
    rgb = freenect->getRgbData();
    coloredDepth = freenect->getColoredDepthData();
    pointCloud = freenect->get6DPointCloudData();

    osg::Geometry* geometry = rgbd3DNode->getDrawable(0)->asGeometry();

    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(geometry->getVertexArray());
    osg::Vec4Array* colors = static_cast<osg::Vec4Array*>(geometry->getColorArray());
    for (int i = 0; i < pointCloud.size(); ++i)
    {
        double x = pointCloud[i].x;
        double y = pointCloud[i].y;
        double z = pointCloud[i].z;
        (*vertices)[i].set(x, z, -y);

        if (enableRGBDColor)
        {
            (*colors)[i].set(pointCloud[i].r / 255.0f,
                             pointCloud[i].g / 255.0f,
                             pointCloud[i].b / 255.0f,
                             1.0f);
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
                                                      0, pointCloud.size()));
    }
    else
    {
        osg::DrawArrays* drawarrays = static_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0));
        drawarrays->setCount(pointCloud.size());
    }
}
#endif

void
Pixhawk3DWidget::markTarget(void)
{
    double robotZ = 0.0f;
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

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
#include <osgText/Text>

#include "../MainWindow.h"
#include "PixhawkCheetahGeode.h"
#include "TerrainParamDialog.h"
#include "UASManager.h"

#include "QGC.h"
#include "gpl.h"

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
#include <tr1/memory>
#include <pixhawk/pixhawk.pb.h>
#endif

Pixhawk3DWidget::Pixhawk3DWidget(QWidget* parent)
 : kMessageTimeout(4.0)
 , mMode(DEFAULT_MODE)
 , mSelectedWpIndex(-1)
 , mActiveSystemId(-1)
 , mActiveUAS(NULL)
 , mGlobalViewParams(new GlobalViewParams)
 , mFollowCameraId(-1)
 , mInitCameraPos(false)
 , m3DWidget(new Q3DWidget(this))
 , mViewParamWidget(new ViewParamWidget(mGlobalViewParams, mSystemViewParamMap, this, parent))
{
    connect(m3DWidget, SIGNAL(sizeChanged(int,int)), this, SLOT(sizeChanged(int,int)));
    connect(m3DWidget, SIGNAL(update()), this, SLOT(update()));

    m3DWidget->setCameraParams(2.0f, 30.0f, 0.01f, 10000.0f);
    m3DWidget->init(15.0f);
    m3DWidget->handleDeviceEvents() = false;

    mWorldGridNode = createWorldGrid();
    m3DWidget->worldMap()->addChild(mWorldGridNode, false);

    mTerrainPAT = new osg::PositionAttitudeTransform;
    m3DWidget->worldMap()->addChild(mTerrainPAT);

    // generate map model
    mImageryNode = createImagery();
    mImageryNode->setName("imagery");
    m3DWidget->worldMap()->addChild(mImageryNode, false);

    setupHUD();

    buildLayout();

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(activeSystemChanged(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
            this, SLOT(systemCreated(UASInterface*)));
    connect(mGlobalViewParams.data(), SIGNAL(followCameraChanged(int)),
            this, SLOT(followCameraChanged(int)));
    connect(mGlobalViewParams.data(), SIGNAL(imageryParamsChanged(void)),
            this, SLOT(imageryParamsChanged(void)));

    MainWindow* parentWindow = qobject_cast<MainWindow*>(parent);
    parentWindow->addDockWidget(Qt::LeftDockWidgetArea, mViewParamWidget);

    mViewParamWidget->hide();

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

Pixhawk3DWidget::~Pixhawk3DWidget()
{

}

void
Pixhawk3DWidget::activeSystemChanged(UASInterface* uas)
{
    mActiveSystemId = uas->getUASID();

    mActiveUAS = uas;

    mMode = DEFAULT_MODE;
}

void
Pixhawk3DWidget::systemCreated(UASInterface *uas)
{
    int systemId = uas->getUASID();

    if (mSystemContainerMap.contains(systemId))
    {
        return;
    }

    mSystemViewParamMap.insert(systemId, SystemViewParamsPtr(new SystemViewParams(systemId)));
    mSystemContainerMap.insert(systemId, SystemContainer());

    connect(uas, SIGNAL(localPositionChanged(UASInterface*,int,double,double,double,quint64)),
            this, SLOT(localPositionChanged(UASInterface*,int,double,double,double,quint64)));
    connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),
            this, SLOT(localPositionChanged(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)),
            this, SLOT(attitudeChanged(UASInterface*,int,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),
            this, SLOT(attitudeChanged(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(userPositionSetPointsChanged(int,float,float,float,float)),
            this, SLOT(setpointChanged(int,float,float,float,float)));
    connect(uas, SIGNAL(homePositionChanged(int,double,double,double)),
            this, SLOT(homePositionChanged(int,double,double,double)));
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    connect(uas, SIGNAL(overlayChanged(UASInterface*)),
            this, SLOT(addOverlay(UASInterface*)));
#endif

//    mSystemContainerMap[systemId].gpsLocalOrigin() = QVector3D(47.419182, 8.566980, 428);
    initializeSystem(systemId, uas->getColor());

    emit systemCreatedSignal(uas);
}

void
Pixhawk3DWidget::localPositionChanged(UASInterface* uas, int component,
                                      double x, double y, double z,
                                      quint64 time)
{
    Q_UNUSED(time);

    int systemId = uas->getUASID();

    if (!mSystemContainerMap.contains(systemId))
    {
        return;
    }

    SystemContainer& systemData = mSystemContainerMap[systemId];

    // update trail data
    if (!systemData.trailMap().contains(component))
    {
        systemData.trailMap().insert(component, QVector<osg::Vec3d>());
        systemData.trailMap()[component].reserve(10000);
        systemData.trailIndexMap().insert(component,
                                          systemData.trailMap().size() - 1);

        // generate nice bright random color
        float golden_ratio_conjugate = 0.618033988749895f;

        float h = (float)qrand() / RAND_MAX + golden_ratio_conjugate;
        if (h > 1.0f)
        {
            h -= 1.0f;
        }

        QColor colorHSV;
        colorHSV.setHsvF(h, 0.99, 0.99, 0.5);

        QColor colorRGB = colorHSV.toRgb();

        osg::Vec4f color(colorRGB.redF(), colorRGB.greenF(), colorRGB.blueF(), colorRGB.alphaF());

        systemData.trailNode()->addDrawable(createTrail(color));
        systemData.trailNode()->addDrawable(createLink(uas->getColor()));

        double radius = 0.5;

        osg::ref_ptr<osg::Group> group = new osg::Group;

        // cone indicates orientation
        osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable;
        double coneRadius = radius / 2.0;
        osg::ref_ptr<osg::Cone> cone =
            new osg::Cone(osg::Vec3d(0.0, 0.0, 0.0),
                          coneRadius, radius * 2.0);

        sd->setShape(cone);
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->setColor(color);

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(sd);

        osg::ref_ptr<osg::PositionAttitudeTransform> pat =
            new osg::PositionAttitudeTransform;
        pat->addChild(geode);
        pat->setAttitude(osg::Quat(- M_PI_2, osg::Vec3d(1.0f, 0.0f, 0.0f),
                                   M_PI_2, osg::Vec3d(0.0f, 1.0f, 0.0f),
                                   0.0, osg::Vec3d(0.0f, 0.0f, 1.0f)));
        group->addChild(pat);

        // cylinder indicates position
        sd = new osg::ShapeDrawable;
        osg::ref_ptr<osg::Cylinder> cylinder =
            new osg::Cylinder(osg::Vec3d(0.0, 0.0, 0.0),
                              radius, 0);

        sd->setShape(cylinder);
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->setColor(color);

        geode = new osg::Geode;
        geode->addDrawable(sd);
        group->addChild(geode);

        // text indicates component id
        colorRGB.lighter();
        color = osg::Vec4(colorRGB.redF(), colorRGB.greenF(), colorRGB.blueF(), 1.0f);

        osg::ref_ptr<osgText::Text> text = new osgText::Text;
        text->setFont(m3DWidget->font());
        text->setText(QString::number(component).toStdString().c_str());
        text->setColor(color);
        text->setCharacterSize(0.3f);
        text->setAxisAlignment(osgText::Text::XY_PLANE);
        text->setAlignment(osgText::Text::CENTER_CENTER);
        text->setPosition(osg::Vec3(0.0, -0.8, 0.0));

        sd = new osg::ShapeDrawable;
        osg::ref_ptr<osg::Box> textBox =
            new osg::Box(osg::Vec3(0.0, -0.8, -0.01), 0.7, 0.4, 0.01);

        sd->setShape(textBox);
        sd->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        sd->setColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));

        geode = new osg::Geode;
        geode->addDrawable(text);
        geode->addDrawable(sd);
        group->addChild(geode);

        pat = new osg::PositionAttitudeTransform;
        pat->addChild(group);
        systemData.orientationNode()->addChild(pat);
    }

    QVector<osg::Vec3d>& trail = systemData.trailMap()[component];

    bool addToTrail = false;
    if (trail.size() > 0)
    {
        if (fabs(x - trail[trail.size() - 1].x()) > 0.01f ||
                fabs(y - trail[trail.size() - 1].y()) > 0.01f ||
                fabs(z - trail[trail.size() - 1].z()) > 0.01f)
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
        osg::Vec3d p(x, y, z);
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
}

void
Pixhawk3DWidget::localPositionChanged(UASInterface* uas,
                                      double x, double y, double z,
                                      quint64 time)
{
    Q_UNUSED(time);

    int systemId = uas->getUASID();

    if (!mSystemContainerMap.contains(systemId))
    {
        return;
    }

//    // Add offset
//    UAS* mav = qobject_cast<UAS*>(uas);

//    float offX = mav->getNedPosGlobalOffset().x();
//    float offY = mav->getNedPosGlobalOffset().y();
//    float offZ = mav->getNedPosGlobalOffset().z();
//    float offYaw = mav->getNedAttGlobalOffset().z();

    // update system position
    m3DWidget->systemGroup(systemId)->position()->setPosition(osg::Vec3d(y, x, -z));
}

void
Pixhawk3DWidget::attitudeChanged(UASInterface* uas, int component,
                                 double roll, double pitch, double yaw,
                                 quint64 time)
{
    Q_UNUSED(roll);
    Q_UNUSED(pitch);
    Q_UNUSED(time);

    int systemId = uas->getUASID();

    if (!mSystemContainerMap.contains(systemId))
    {
        return;
    }

    SystemContainer& systemData = mSystemContainerMap[systemId];

    // update trail data
    if (!systemData.trailMap().contains(component))
    {
        return;
    }

    int idx = systemData.trailIndexMap().value(component);

    osg::PositionAttitudeTransform* pat =
        dynamic_cast<osg::PositionAttitudeTransform*>(systemData.orientationNode()->getChild(idx));

    pat->setAttitude(osg::Quat(-yaw, osg::Vec3d(0.0f, 0.0f, 1.0f),
                               0.0, osg::Vec3d(1.0f, 0.0f, 0.0f),
                               0.0, osg::Vec3d(0.0f, 1.0f, 0.0f)));
}

void
Pixhawk3DWidget::attitudeChanged(UASInterface* uas,
                                 double roll, double pitch, double yaw,
                                 quint64 time)
{
    Q_UNUSED(time);

    int systemId = uas->getUASID();

    if (!mSystemContainerMap.contains(systemId))
    {
        return;
    }

    // update system attitude
    osg::Quat q(-yaw, osg::Vec3d(0.0f, 0.0f, 1.0f),
                pitch, osg::Vec3d(1.0f, 0.0f, 0.0f),
                roll, osg::Vec3d(0.0f, 1.0f, 0.0f));
    m3DWidget->systemGroup(systemId)->attitude()->setAttitude(q);
}

void
Pixhawk3DWidget::homePositionChanged(int uasId, double lat, double lon,
                                     double alt)
{
    if (!mSystemContainerMap.contains(uasId))
    {
        return;
    }

    SystemContainer& systemData = mSystemContainerMap[uasId];

    systemData.gpsLocalOrigin() = QVector3D(lat, lon, alt);
}

void
Pixhawk3DWidget::setpointChanged(int uasId, float x, float y, float z,
                                 float yaw)
{
    if (!mSystemContainerMap.contains(uasId))
    {
        return;
    }

    UASInterface* uas = UASManager::instance()->getUASForId(uasId);
    if (!uas)
    {
        return;
    }

    QColor color = uas->getColor();
    const SystemViewParamsPtr& systemViewParams = mSystemViewParamMap.value(uasId);

    osg::ref_ptr<osg::PositionAttitudeTransform> pat =
        new osg::PositionAttitudeTransform;

    pat->setPosition(osg::Vec3d(y, x, -z));
    pat->setAttitude(osg::Quat(osg::DegreesToRadians(yaw) - M_PI_2, osg::Vec3d(1.0f, 0.0f, 0.0f),
                               M_PI_2, osg::Vec3d(0.0f, 1.0f, 0.0f),
                               0.0, osg::Vec3d(0.0f, 0.0f, 1.0f)));

    osg::ref_ptr<osg::Cone> cone = new osg::Cone(osg::Vec3f(0.0f, 0.0f, 0.0f), 0.1f, 0.3f);
    osg::ref_ptr<osg::ShapeDrawable> coneDrawable = new osg::ShapeDrawable(cone);
    coneDrawable->setColor(osg::Vec4f(color.redF(), color.greenF(), color.blueF(), 1.0f));
    coneDrawable->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    osg::ref_ptr<osg::Geode> coneGeode = new osg::Geode;
    coneGeode->addDrawable(coneDrawable);

    pat->addChild(coneGeode);

    osg::ref_ptr<osg::Group>& setpointGroupNode = mSystemContainerMap[uasId].setpointGroupNode();

    setpointGroupNode->addChild(pat);
    if (setpointGroupNode->getNumChildren() > static_cast<unsigned int>(systemViewParams->setpointHistoryLength()))
    {
        setpointGroupNode->removeChildren(0, setpointGroupNode->getNumChildren() - systemViewParams->setpointHistoryLength());
    }

    osg::Vec4f setpointColor(color.redF(), color.greenF(), color.blueF(), 1.0f);
    int setpointCount = setpointGroupNode->getNumChildren();

    // update colors
    for (int i = 0; i < setpointCount; ++i)
    {
        osg::PositionAttitudeTransform* pat =
            dynamic_cast<osg::PositionAttitudeTransform*>(setpointGroupNode->getChild(i));

        osg::Geode* geode = dynamic_cast<osg::Geode*>(pat->getChild(0));
        osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>(geode->getDrawable(0));

        setpointColor.a() = static_cast<float>(i + 1) / setpointCount;
        sd->setColor(setpointColor);
    }
}

void
Pixhawk3DWidget::clearData(void)
{
    QMutableMapIterator<int, SystemContainer> it(mSystemContainerMap);
    while (it.hasNext())
    {
        it.next();

        SystemContainer& systemData = it.value();

        // clear setpoint data
        systemData.setpointGroupNode()->removeChildren(0, systemData.setpointGroupNode()->getNumChildren());

        // clear trail data
        systemData.trailMap().clear();
    }
}

void
Pixhawk3DWidget::showTerrainParamWindow(void)
{
    TerrainParamDialog::getTerrainParams(mGlobalViewParams);

    const QVector3D& positionOffset = mGlobalViewParams->terrainPositionOffset();
    const QVector3D& attitudeOffset = mGlobalViewParams->terrainAttitudeOffset();

    mTerrainPAT->setPosition(osg::Vec3d(positionOffset.y(), positionOffset.x(), -positionOffset.z()));
    mTerrainPAT->setAttitude(osg::Quat(- attitudeOffset.z(), osg::Vec3d(0.0f, 0.0f, 1.0f),
                                       attitudeOffset.y(), osg::Vec3d(1.0f, 0.0f, 0.0f),
                                       attitudeOffset.x(), osg::Vec3d(0.0f, 1.0f, 0.0f)));
}

void
Pixhawk3DWidget::showViewParamWindow(void)
{
    if (mViewParamWidget->isVisible())
    {
        mViewParamWidget->hide();
    }
    else
    {
        mViewParamWidget->show();
    }
}

void
Pixhawk3DWidget::followCameraChanged(int systemId)
{
    if (systemId == -1)
    {
        mFollowCameraId = -1;
    }

    UASInterface* uas = UASManager::instance()->getUASForId(systemId);
    if (!uas)
    {
        return;
    }

    if (mFollowCameraId != systemId)
    {
        double x = 0.0, y = 0.0, z = 0.0;
        getPosition(uas, mGlobalViewParams->frame(), x, y, z);

        mCameraPos = QVector3D(x, y, z);

        m3DWidget->recenterCamera(y, x, -z);

        mFollowCameraId = systemId;
    }
}

void
Pixhawk3DWidget::imageryParamsChanged(void)
{
    mImageryNode->setImageryType(mGlobalViewParams->imageryType());
    mImageryNode->setPath(mGlobalViewParams->imageryPath());

    const QVector3D& offset = mGlobalViewParams->imageryOffset();
    mImageryNode->setOffset(offset.x(), offset.y(), offset.z());
}

void
Pixhawk3DWidget::recenterActiveCamera(void)
{
    if (mFollowCameraId != -1)
    {
        UASInterface* uas = UASManager::instance()->getUASForId(mFollowCameraId);
        if (!uas)
        {
            return;
        }

        double x = 0.0, y = 0.0, z = 0.0;
        getPosition(uas, mGlobalViewParams->frame(), x, y, z);

        mCameraPos = QVector3D(x, y, z);

        m3DWidget->recenterCamera(y, x, -z);
    }
}

void
Pixhawk3DWidget::modelChanged(int systemId, int index)
{
    if (!mSystemContainerMap.contains(systemId))
    {
        return;
    }

    SystemContainer& systemData = mSystemContainerMap[systemId];
    osg::ref_ptr<SystemGroupNode>& systemGroupNode = m3DWidget->systemGroup(systemId);

    systemGroupNode->egocentricMap()->removeChild(systemData.modelNode());
    systemData.modelNode() = systemData.models().at(index);
    systemGroupNode->egocentricMap()->addChild(systemData.modelNode());
}

void
Pixhawk3DWidget::setBirdEyeView(void)
{
    mViewParamWidget->setFollowCameraId(-1);

    m3DWidget->rotateCamera(0.0, 0.0, 0.0);
    m3DWidget->setCameraDistance(100.0);
}

void
Pixhawk3DWidget::loadTerrainModel(void)
{
    QString filename = QFileDialog::getOpenFileName(this, "Load Terrain Model",
                                                    QDesktopServices::storageLocation(QDesktopServices::DesktopLocation),
                                                    tr("Collada (*.dae)"));

    if (filename.isNull())
    {
        return;
    }

    osg::ref_ptr<osg::Node> node =
        osgDB::readNodeFile(filename.toStdString().c_str());

    if (node)
    {
        if (mTerrainNode.get())
        {
            mTerrainPAT->removeChild(mTerrainNode);
        }
        mTerrainNode = node;
        mTerrainNode->setName("terrain");
        mTerrainPAT->addChild(mTerrainNode);

        mGlobalViewParams->terrainPositionOffset() = QVector3D();
        mGlobalViewParams->terrainAttitudeOffset() = QVector3D();

        mTerrainPAT->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
        mTerrainPAT->setAttitude(osg::Quat(0.0, osg::Vec3d(0.0f, 0.0f, 1.0f),
                                           0.0, osg::Vec3d(1.0f, 0.0f, 0.0f),
                                           0.0, osg::Vec3d(0.0f, 1.0f, 0.0f)));
    }
    else
    {
        QMessageBox msgBox(QMessageBox::Warning,
                           "Error loading model",
                           QString("Error: Unable to load terrain model (%1).").arg(filename));
        msgBox.exec();
    }
}

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
void
Pixhawk3DWidget::addOverlay(UASInterface *uas)
{
    int systemId = uas->getUASID();

    if (!mSystemContainerMap.contains(systemId))
    {
        return;
    }

    SystemContainer& systemData = mSystemContainerMap[systemId];

    qreal receivedTimestamp;
    px::GLOverlay overlay = uas->getOverlay(receivedTimestamp);

    QString overlayName = QString::fromStdString(overlay.name());

    osg::ref_ptr<SystemGroupNode>& systemNode = m3DWidget->systemGroup(systemId);

    if (!systemData.overlayNodeMap().contains(overlayName))
    {
        osg::ref_ptr<GLOverlayGeode> overlayNode = new GLOverlayGeode;
        systemData.overlayNodeMap().insert(overlayName, overlayNode);

        systemNode->allocentricMap()->addChild(overlayNode, false);
        systemNode->rollingMap()->addChild(overlayNode, false);

        emit overlayCreatedSignal(systemId, overlayName);
    }

    osg::ref_ptr<GLOverlayGeode>& overlayNode = systemData.overlayNodeMap()[overlayName];
    overlayNode->setOverlay(overlay);
    overlayNode->setMessageTimestamp(receivedTimestamp);
}
#endif

void
Pixhawk3DWidget::selectTargetHeading(void)
{
    if (!mActiveUAS)
    {
        return;
    }

    osg::Vec2d p;

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        double altitude = mActiveUAS->getAltitude();

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), altitude);

        p.set(cursorWorldCoords.x(), cursorWorldCoords.y());
    }
    else if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
    {
        double z = mActiveUAS->getLocalZ();

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

        p.set(cursorWorldCoords.x(), cursorWorldCoords.y());
    }

    SystemContainer& systemData = mSystemContainerMap[mActiveUAS->getUASID()];
    QVector4D& target = systemData.target();

    target.setW(atan2(p.y() - target.y(), p.x() - target.x()));
}

void
Pixhawk3DWidget::selectTarget(void)
{
    if (!mActiveUAS)
    {
        return;
    }
    if (!mActiveUAS->getParamManager())
    {
        return;
    }

    SystemContainer& systemData = mSystemContainerMap[mActiveUAS->getUASID()];
    QVector4D& target = systemData.target();

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        double altitude = mActiveUAS->getAltitude();

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(mCachedMousePos, altitude);

        QVariant zTarget;
        if (!mActiveUAS->getParamManager()->getParameterValue(MAV_COMP_ID_PATHPLANNER, "TARGET-ALT", zTarget))
        {
            zTarget = -altitude;
        }

        target = QVector4D(cursorWorldCoords.x(), cursorWorldCoords.y(),
                           zTarget.toReal(), 0.0);
    }
    else if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
    {
        double z = mActiveUAS->getLocalZ();

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(mCachedMousePos, -z);

        QVariant zTarget;
        if (!mActiveUAS->getParamManager()->getParameterValue(MAV_COMP_ID_PATHPLANNER, "TARGET-ALT", zTarget))
        {
            zTarget = z;
        }

        target = QVector4D(cursorWorldCoords.x(), cursorWorldCoords.y(),
                           zTarget.toReal(), 0.0);
    }

    int systemId = mActiveUAS->getUASID();

    QMap<int, SystemViewParamsPtr>::iterator it = mSystemViewParamMap.find(systemId);
    if (it != mSystemViewParamMap.end())
    {
        it.value()->displayTarget() = true;
    }

    mMode = SELECT_TARGET_HEADING_MODE;
}

void
Pixhawk3DWidget::setTarget(void)
{
    selectTargetHeading();

    SystemContainer& systemData = mSystemContainerMap[mActiveUAS->getUASID()];
    QVector4D& target = systemData.target();

    mActiveUAS->setTargetPosition(target.x(), target.y(), target.z(),
                                  osg::RadiansToDegrees(target.w()));
}

void
Pixhawk3DWidget::insertWaypoint(void)
{
    if (!mActiveUAS)
    {
        return;
    }

    Waypoint* wp = NULL;
    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        double latitude = mActiveUAS->getLatitude();
        double longitude = mActiveUAS->getLongitude();
        double altitude = mActiveUAS->getAltitude();
        double x, y;
        QString utmZone;
        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(mCachedMousePos, altitude);

        Imagery::UTMtoLL(cursorWorldCoords.x(), cursorWorldCoords.y(), utmZone,
                         latitude, longitude);

        wp = new Waypoint(0, longitude, latitude, altitude, 0.0, 0.25);
    }
    else if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
    {
        double z = mActiveUAS->getLocalZ();

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(mCachedMousePos, -z);

        wp = new Waypoint(0, cursorWorldCoords.x(),
                          cursorWorldCoords.y(), z, 0.0, 0.25);
    }

    if (wp)
    {
        wp->setFrame(mGlobalViewParams->frame());
        mActiveUAS->getWaypointManager()->addWaypointEditable(wp);
    }

    mSelectedWpIndex = wp->getId();
    mMode = MOVE_WAYPOINT_HEADING_MODE;
}

void
Pixhawk3DWidget::moveWaypointPosition(void)
{
    if (mMode != MOVE_WAYPOINT_POSITION_MODE)
    {
        mMode = MOVE_WAYPOINT_POSITION_MODE;
        return;
    }

    if (!mActiveUAS)
    {
        return;
    }

    const QVector<Waypoint *> waypoints =
        mActiveUAS->getWaypointManager()->getWaypointEditableList();
    Waypoint* waypoint = waypoints.at(mSelectedWpIndex);

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        double latitude = mActiveUAS->getLatitude();
        double longitude = mActiveUAS->getLongitude();
        double altitude = mActiveUAS->getAltitude();
        double x, y;
        QString utmZone;
        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), altitude);

        Imagery::UTMtoLL(cursorWorldCoords.x(), cursorWorldCoords.y(),
                         utmZone, latitude, longitude);

        waypoint->setX(longitude);
        waypoint->setY(latitude);
    }
    else if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
    {
        double z = mActiveUAS->getLocalZ();

        QPointF cursorWorldCoords =
            m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

        waypoint->setX(cursorWorldCoords.x());
        waypoint->setY(cursorWorldCoords.y());
    }
}

void
Pixhawk3DWidget::moveWaypointHeading(void)
{
    if (mMode != MOVE_WAYPOINT_HEADING_MODE)
    {
        mMode = MOVE_WAYPOINT_HEADING_MODE;
        return;
    }

    if (!mActiveUAS)
    {
        return;
    }

    const QVector<Waypoint *> waypoints =
        mActiveUAS->getWaypointManager()->getWaypointEditableList();
    Waypoint* waypoint = waypoints.at(mSelectedWpIndex);

    double x = 0.0, y = 0.0, z = 0.0;

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        double latitude = waypoint->getY();
        double longitude = waypoint->getX();
        z = -waypoint->getZ();
        QString utmZone;
        Imagery::LLtoUTM(latitude, longitude, x, y, utmZone);
    }
    else if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
    {
        z = mActiveUAS->getLocalZ();
    }

    QPointF cursorWorldCoords =
        m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

    double yaw = atan2(cursorWorldCoords.y() - waypoint->getY(),
                       cursorWorldCoords.x() - waypoint->getX());
    yaw = osg::RadiansToDegrees(yaw);

    waypoint->setYaw(yaw);
}

void
Pixhawk3DWidget::deleteWaypoint(void)
{
    if (mActiveUAS)
    {
        mActiveUAS->getWaypointManager()->removeWaypoint(mSelectedWpIndex);
    }
}

void
Pixhawk3DWidget::setWaypointAltitude(void)
{
    if (!mActiveUAS)
    {
        return;
    }

    bool ok;
    const QVector<Waypoint *> waypoints =
        mActiveUAS->getWaypointManager()->getWaypointEditableList();
    Waypoint* waypoint = waypoints.at(mSelectedWpIndex);

    double altitude = waypoint->getZ();
    if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
    {
        altitude = -altitude;
    }

    double newAltitude =
        QInputDialog::getDouble(this, tr("Set altitude of waypoint %1").arg(mSelectedWpIndex),
                                tr("Altitude (m):"), waypoint->getZ(), -1000.0, 1000.0, 1, &ok);
    if (ok)
    {
        if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
        {
            waypoint->setZ(newAltitude);
        }
        else if (mGlobalViewParams->frame() == MAV_FRAME_LOCAL_NED)
        {
            waypoint->setZ(-newAltitude);
        }
    }
}

void
Pixhawk3DWidget::clearAllWaypoints(void)
{
    if (mActiveUAS)
    {
        const QVector<Waypoint *> waypoints =
            mActiveUAS->getWaypointManager()->getWaypointEditableList();
        for (int i = waypoints.size() - 1; i >= 0; --i)
        {
            mActiveUAS->getWaypointManager()->removeWaypoint(i);
        }
    }
}

void
Pixhawk3DWidget::moveImagery(void)
{
    if (mMode != MOVE_IMAGERY_MODE)
    {
        mMode = MOVE_IMAGERY_MODE;
        return;
    }

    if (!mActiveUAS)
    {
        return;
    }

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        return;
    }

    double z = mActiveUAS->getLocalZ();

    QPointF cursorWorldCoords =
        m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

    QVector3D& offset = mGlobalViewParams->imageryOffset();
    offset.setX(cursorWorldCoords.x());
    offset.setY(cursorWorldCoords.y());

    mImageryNode->setOffset(offset.x(), offset.y(), offset.z());
}

void
Pixhawk3DWidget::moveTerrain(void)
{
    if (mMode != MOVE_TERRAIN_MODE)
    {
        mMode = MOVE_TERRAIN_MODE;
        return;
    }

    if (!mActiveUAS)
    {
        return;
    }

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        return;
    }

    double z = mActiveUAS->getLocalZ();

    QPointF cursorWorldCoords =
        m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

    QVector3D& positionOffset = mGlobalViewParams->terrainPositionOffset();
    positionOffset.setX(cursorWorldCoords.x());
    positionOffset.setY(cursorWorldCoords.y());

    mTerrainPAT->setPosition(osg::Vec3d(positionOffset.y(), positionOffset.x(), -positionOffset.z()));
}

void
Pixhawk3DWidget::rotateTerrain(void)
{
    if (mMode != ROTATE_TERRAIN_MODE)
    {
        mMode = ROTATE_TERRAIN_MODE;
        return;
    }

    if (!mActiveUAS)
    {
        return;
    }

    if (mGlobalViewParams->frame() == MAV_FRAME_GLOBAL)
    {
        return;
    }

    double z = mActiveUAS->getLocalZ();

    QPointF cursorWorldCoords =
        m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

    const QVector3D& positionOffset = mGlobalViewParams->terrainPositionOffset();
    QVector3D& attitudeOffset = mGlobalViewParams->terrainAttitudeOffset();

    double yaw = atan2(cursorWorldCoords.y() - positionOffset.y(),
                       cursorWorldCoords.x() - positionOffset.x());

    attitudeOffset.setZ(yaw);

    mTerrainPAT->setAttitude(osg::Quat(- attitudeOffset.z(), osg::Vec3d(0.0f, 0.0f, 1.0f),
                                       attitudeOffset.y(), osg::Vec3d(1.0f, 0.0f, 0.0f),
                                       attitudeOffset.x(), osg::Vec3d(0.0f, 1.0f, 0.0f)));
}

void
Pixhawk3DWidget::sizeChanged(int width, int height)
{
    resizeHUD(width, height);
}

void
Pixhawk3DWidget::update(void)
{
    MAV_FRAME frame = mGlobalViewParams->frame();

    // set node visibility
    m3DWidget->worldMap()->setChildValue(mTerrainPAT,
                                         mGlobalViewParams->displayTerrain());
    m3DWidget->worldMap()->setChildValue(mWorldGridNode,
                                         mGlobalViewParams->displayWorldGrid());
    if (mGlobalViewParams->imageryType() == Imagery::BLANK_MAP)
    {
        m3DWidget->worldMap()->setChildValue(mImageryNode, false);
    }
    else
    {
        m3DWidget->worldMap()->setChildValue(mImageryNode, true);
    }

    // set system-specific node visibility
    QMutableMapIterator<int, SystemViewParamsPtr> it(mSystemViewParamMap);
    while (it.hasNext())
    {
        it.next();

        osg::ref_ptr<SystemGroupNode>& systemNode = m3DWidget->systemGroup(it.key());
        SystemContainer& systemData = mSystemContainerMap[it.key()];
        const SystemViewParamsPtr& systemViewParams = it.value();

        osg::ref_ptr<osg::Switch>& allocentricMap = systemNode->allocentricMap();
        allocentricMap->setChildValue(systemData.setpointGroupNode(),
                                      systemViewParams->displaySetpoints());

        osg::ref_ptr<osg::Switch>& rollingMap = systemNode->rollingMap();
        rollingMap->setChildValue(systemData.localGridNode(),
                                  systemViewParams->displayLocalGrid());
        rollingMap->setChildValue(systemData.orientationNode(),
                                  systemViewParams->displayTrails());
        rollingMap->setChildValue(systemData.pointCloudNode(),
                                  systemViewParams->displayPointCloud());
        rollingMap->setChildValue(systemData.targetNode(),
                                  systemViewParams->displayTarget());
        rollingMap->setChildValue(systemData.trailNode(),
                                  systemViewParams->displayTrails());
        rollingMap->setChildValue(systemData.waypointGroupNode(),
                                  systemViewParams->displayWaypoints());

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
        rollingMap->setChildValue(systemData.obstacleGroupNode(),
                                  systemViewParams->displayObstacleList());

        QMutableMapIterator<QString, osg::ref_ptr<GLOverlayGeode> > itOverlay(systemData.overlayNodeMap());
        while (itOverlay.hasNext())
        {
            itOverlay.next();

            osg::ref_ptr<GLOverlayGeode>& overlayNode = itOverlay.value();

            bool displayOverlay = systemViewParams->displayOverlay().value(itOverlay.key());

            bool visible;
            visible = (overlayNode->coordinateFrameType() == px::GLOverlay::GLOBAL) &&
                      displayOverlay &&
                      (QGC::groundTimeSeconds() - overlayNode->messageTimestamp() < kMessageTimeout);

            allocentricMap->setChildValue(overlayNode, visible);

            visible = (overlayNode->coordinateFrameType() == px::GLOverlay::LOCAL) &&
                      displayOverlay &&
                      (QGC::groundTimeSeconds() - overlayNode->messageTimestamp() < kMessageTimeout);;

            rollingMap->setChildValue(overlayNode, visible);
        }

        rollingMap->setChildValue(systemData.plannedPathNode(),
                                  systemViewParams->displayPlannedPath());

        m3DWidget->hudGroup()->setChildValue(systemData.depthImageNode(),
                                             systemViewParams->displayRGBD());
        m3DWidget->hudGroup()->setChildValue(systemData.rgbImageNode(),
                                             systemViewParams->displayRGBD());
#endif
    }

    if (mFollowCameraId != -1)
    {
        UASInterface* uas = UASManager::instance()->getUASForId(mFollowCameraId);
        if (uas)
        {
            double x = 0.0, y = 0.0, z = 0.0;
            getPosition(uas, mGlobalViewParams->frame(), x, y, z);

            double dx = y - mCameraPos.y();
            double dy = x - mCameraPos.x();
            double dz = mCameraPos.z() - z;

            m3DWidget->moveCamera(dx, dy, dz);

            mCameraPos = QVector3D(x, y, z);
        }
    }
    else
    {
        if (!mInitCameraPos && mActiveUAS)
        {
            double x = 0.0, y = 0.0, z = 0.0;
            getPosition(mActiveUAS, frame, x, y, z);
            m3DWidget->recenterCamera(y, x, -z);

            mCameraPos = QVector3D(x, y, z);

            setBirdEyeView();
            mInitCameraPos = true;
        }
    }

    // update system-specific data
    it.toFront();
    while (it.hasNext())
    {
        it.next();

        int systemId = it.key();

        UASInterface* uas = UASManager::instance()->getUASForId(systemId);

        SystemContainer& systemData = mSystemContainerMap[systemId];
        SystemViewParamsPtr& systemViewParams = it.value();

        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double roll = 0.0;
        double pitch = 0.0;
        double yaw = 0.0;

        getPose(uas, frame, x, y, z, roll, pitch, yaw);

        if (systemViewParams->displaySetpoints())
        {

        }
        else
        {
            systemData.setpointGroupNode()->removeChildren(0, systemData.setpointGroupNode()->getNumChildren());
        }
        if (systemViewParams->displayTarget())
        {
            if (systemData.target().isNull())
            {
                systemViewParams->displayTarget() = false;
            }
            else
            {
                updateTarget(uas, frame, x, y, z, systemData.target(),
                             systemData.targetNode());
            }
        }
        if (systemViewParams->displayTrails())
        {
            updateTrails(x, y, z, systemData.trailNode(), systemData.orientationNode(),
                         systemData.trailMap(), systemData.trailIndexMap());
        }
        else
        {
            systemData.trailMap().clear();
        }
        if (systemViewParams->displayWaypoints())
        {
            updateWaypoints(uas, frame, systemData.waypointGroupNode());
        }

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
        if (systemViewParams->displayObstacleList())
        {
            updateObstacles(uas, frame, x, y, z, systemData.obstacleGroupNode());
        }
        if (systemViewParams->displayPlannedPath())
        {
            updatePlannedPath(uas, frame, x, y, z, systemData.plannedPathNode());
        }
        if (systemViewParams->displayPointCloud())
        {
            updatePointCloud(uas, frame, x, y, z, systemData.pointCloudNode(),
                             systemViewParams->colorPointCloudByDistance());
        }
        if (systemViewParams->displayRGBD())
        {
            updateRGBD(uas, frame, systemData.rgbImageNode(),
                       systemData.depthImageNode());
        }

        if (frame == MAV_FRAME_LOCAL_NED &&
            mGlobalViewParams->imageryType() != Imagery::BLANK_MAP &&
            !systemData.gpsLocalOrigin().isNull() &&
            mActiveUAS->getUASID() == systemId)
        {
            const QVector3D& gpsLocalOrigin = systemData.gpsLocalOrigin();

            double utmX, utmY;
            QString utmZone;
            Imagery::LLtoUTM(gpsLocalOrigin.x(), gpsLocalOrigin.y(), utmX, utmY, utmZone);

            updateImagery(utmX, utmY, utmZone, frame);
        }
#endif
    }

    if (frame == MAV_FRAME_GLOBAL &&
        mGlobalViewParams->imageryType() != Imagery::BLANK_MAP)
    {
//        updateImagery(x, y, z, utmZone);
    }

    if (mActiveUAS)
    {
      updateHUD(mActiveUAS, frame);
    }

    layout()->update();
}

void
Pixhawk3DWidget::addModels(QVector< osg::ref_ptr<osg::Node> >& models,
                           const QColor& systemColor)
{
    QDir directory("models");
    QStringList files = directory.entryList(QStringList("*.osg"), QDir::Files);

    // add Pixhawk Bravo model
    models.push_back(PixhawkCheetahGeode::create(systemColor));

    // add sphere of 0.05m radius
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3f(0.0f, 0.0f, 0.0f), 0.05f);
    osg::ref_ptr<osg::ShapeDrawable> sphereDrawable = new osg::ShapeDrawable(sphere);
    sphereDrawable->setColor(osg::Vec4f(systemColor.redF(), systemColor.greenF(), systemColor.blueF(), 1.0f));
    osg::ref_ptr<osg::Geode> sphereGeode = new osg::Geode;
    sphereGeode->addDrawable(sphereDrawable);
    sphereGeode->setName("Sphere (0.1m)");
    models.push_back(sphereGeode);

    // add all other models in folder
    for (int i = 0; i < files.size(); ++i)
    {
        osg::ref_ptr<osg::Node> node =
            osgDB::readNodeFile(directory.absoluteFilePath(files[i]).toStdString().c_str());

        if (node)
        {
            models.push_back(node);
        }
        else
        {
            printf("%s\n", QString("ERROR: Could not load file " + directory.absoluteFilePath(files[i]) + "\n").toStdString().c_str());
        }
    }
}

void
Pixhawk3DWidget::buildLayout(void)
{
    QPushButton* clearDataButton = new QPushButton(this);
    clearDataButton->setText("Clear Data");

    QPushButton* viewParamWindowButton = new QPushButton(this);
    viewParamWindowButton->setCheckable(true);
    viewParamWindowButton->setText("View Parameters");

    QHBoxLayout* layoutTop = new QHBoxLayout;
    layoutTop->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    layoutTop->addWidget(clearDataButton);
    layoutTop->addWidget(viewParamWindowButton);

    QPushButton* recenterButton = new QPushButton(this);
    recenterButton->setText("Recenter Camera");

    QPushButton* birdEyeViewButton = new QPushButton(this);
    birdEyeViewButton->setText("Bird's Eye View");

    QPushButton* loadTerrainModelButton = new QPushButton(this);
    loadTerrainModelButton->setText("Load Terrain Model");

    QHBoxLayout* layoutBottom = new QHBoxLayout;
    layoutBottom->addWidget(recenterButton);
    layoutBottom->addWidget(birdEyeViewButton);
    layoutBottom->addItem(new QSpacerItem(10, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    layoutBottom->addWidget(loadTerrainModelButton);

    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addLayout(layoutTop, 0, 0);
    layout->addWidget(m3DWidget, 1, 0);
    layout->addLayout(layoutBottom, 2, 0);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 100);
    layout->setRowStretch(2, 1);

    connect(clearDataButton, SIGNAL(clicked()),
            this, SLOT(clearData()));
    connect(viewParamWindowButton, SIGNAL(clicked()),
            this, SLOT(showViewParamWindow()));
    connect(recenterButton, SIGNAL(clicked()),
            this, SLOT(recenterActiveCamera()));
    connect(birdEyeViewButton, SIGNAL(clicked()),
            this, SLOT(setBirdEyeView()));
    connect(loadTerrainModelButton, SIGNAL(clicked()),
            this, SLOT(loadTerrainModel()));
}

void
Pixhawk3DWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);
    if (event->isAccepted())
    {
        return;
    }

    m3DWidget->handleKeyPressEvent(event);
}

void
Pixhawk3DWidget::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);
    if (event->isAccepted())
    {
        return;
    }

    m3DWidget->handleKeyReleaseEvent(event);
}

void
Pixhawk3DWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if (event->isAccepted())
    {
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        if (mMode == SELECT_TARGET_HEADING_MODE)
        {
            setTarget();
            event->accept();
        }

        if (mMode != DEFAULT_MODE)
        {
            mMode = DEFAULT_MODE;
            event->accept();
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (m3DWidget->getSceneData() && mActiveUAS)
        {
            mSelectedWpIndex = -1;
            bool mouseOverImagery = false;
            bool mouseOverTerrain = false;

            SystemContainer& systemData = mSystemContainerMap[mActiveUAS->getUASID()];
            osg::ref_ptr<WaypointGroupNode>& waypointGroupNode = systemData.waypointGroupNode();

            osgUtil::LineSegmentIntersector::Intersections intersections;

            QPoint widgetMousePos = m3DWidget->mapFromParent(event->pos());

            if (m3DWidget->computeIntersections(widgetMousePos.x(),
                                                m3DWidget->height() - widgetMousePos.y(),
                                                intersections))
            {
                for (osgUtil::LineSegmentIntersector::Intersections::iterator
                     it = intersections.begin(); it != intersections.end(); it++)
                {
                    for (uint i = 0 ; i < it->nodePath.size(); ++i)
                    {
                        osg::Node* node = it->nodePath[i];
                        std::string nodeName = node->getName();

                        if (nodeName.substr(0, 2).compare("wp") == 0)
                        {
                            if (node->getParent(0)->getParent(0) == waypointGroupNode.get())
                            {
                                mSelectedWpIndex = atoi(nodeName.substr(2).c_str());
                            }
                        }
                        else if (nodeName.compare("imagery") == 0)
                        {
                            mouseOverImagery = true;
                        }
                        else if (nodeName.compare("terrain") == 0)
                        {
                            mouseOverTerrain = true;
                        }
                    }
                }
            }

            QMenu menu;
            if (mSelectedWpIndex == -1)
            {
                mCachedMousePos = event->pos();

                menu.addAction("Insert new waypoint", this, SLOT(insertWaypoint()));
            }
            else
            {
                QString text;
                text = QString("Move waypoint %1").arg(QString::number(mSelectedWpIndex));
                menu.addAction(text, this, SLOT(moveWaypointPosition()));

                text = QString("Change heading of waypoint %1").arg(QString::number(mSelectedWpIndex));
                menu.addAction(text, this, SLOT(moveWaypointHeading()));

                text = QString("Change altitude of waypoint %1").arg(QString::number(mSelectedWpIndex));
                menu.addAction(text, this, SLOT(setWaypointAltitude()));

                text = QString("Delete waypoint %1").arg(QString::number(mSelectedWpIndex));
                menu.addAction(text, this, SLOT(deleteWaypoint()));
            }

            menu.addAction("Clear all waypoints", this, SLOT(clearAllWaypoints()));
            menu.addSeparator();
            menu.addAction("Select target", this, SLOT(selectTarget()));

            if (mouseOverImagery)
            {
                menu.addSeparator();
                menu.addAction("Move imagery", this, SLOT(moveImagery()));
            }
            if (mouseOverTerrain)
            {
                menu.addSeparator();
                menu.addAction("Move terrain", this, SLOT(moveTerrain()));
                menu.addAction("Rotate terrain", this, SLOT(rotateTerrain()));
                menu.addAction("Edit terrain parameters", this, SLOT(showTerrainParamWindow()));
            }

            menu.exec(event->globalPos());

            event->accept();
        }
    }


    m3DWidget->handleMousePressEvent(event);
}

void
Pixhawk3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->isAccepted())
    {
        return;
    }

    m3DWidget->handleMouseReleaseEvent(event);
}

void
Pixhawk3DWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    if (event->isAccepted())
    {
        return;
    }

    switch (mMode)
    {
    case SELECT_TARGET_HEADING_MODE:
        selectTargetHeading();
        event->accept();
        break;
    case MOVE_WAYPOINT_POSITION_MODE:
        moveWaypointPosition();
        event->accept();
        break;
    case MOVE_WAYPOINT_HEADING_MODE:
        moveWaypointHeading();
        event->accept();
        break;
    case MOVE_IMAGERY_MODE:
        moveImagery();
        event->accept();
        break;
    case MOVE_TERRAIN_MODE:
        moveTerrain();
        event->accept();
        break;
    case ROTATE_TERRAIN_MODE:
        rotateTerrain();
        event->accept();
        break;
    default:
        {}
    }

    m3DWidget->handleMouseMoveEvent(event);
}

void
Pixhawk3DWidget::wheelEvent(QWheelEvent* event)
{
    QWidget::wheelEvent(event);
    if (event->isAccepted())
    {
        return;
    }

    m3DWidget->handleWheelEvent(event);
}

void
Pixhawk3DWidget::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);

    emit visibilityChanged(true);
}

void
Pixhawk3DWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);

    emit visibilityChanged(false);
}

void
Pixhawk3DWidget::initializeSystem(int systemId, const QColor& systemColor)
{
    SystemViewParamsPtr& systemViewParams = mSystemViewParamMap[systemId];
    SystemContainer& systemData = mSystemContainerMap[systemId];
    osg::ref_ptr<SystemGroupNode>& systemNode = m3DWidget->systemGroup(systemId);

    // generate grid model
    systemData.localGridNode() = createLocalGrid();
    systemNode->rollingMap()->addChild(systemData.localGridNode(), false);

    // generate orientation model
    systemData.orientationNode() = new osg::Group;
    systemNode->rollingMap()->addChild(systemData.orientationNode(), false);

    // generate point cloud model
    systemData.pointCloudNode() = createPointCloud();
    systemNode->rollingMap()->addChild(systemData.pointCloudNode(), false);

    // generate setpoint model
    systemData.setpointGroupNode() = new osg::Group;
    systemNode->allocentricMap()->addChild(systemData.setpointGroupNode(), false);

    // generate target model
    systemData.targetNode() = createTarget(systemColor);
    systemNode->rollingMap()->addChild(systemData.targetNode(), false);

    // generate empty trail model
    systemData.trailNode() = new osg::Geode;
    systemNode->rollingMap()->addChild(systemData.trailNode(), false);

    // generate waypoint model
    systemData.waypointGroupNode() = new WaypointGroupNode(systemColor);
    systemData.waypointGroupNode()->init();
    systemNode->rollingMap()->addChild(systemData.waypointGroupNode(), false);

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    systemData.obstacleGroupNode() = new ObstacleGroupNode;
    systemData.obstacleGroupNode()->init();
    systemNode->rollingMap()->addChild(systemData.obstacleGroupNode(), false);

    // generate path model
    systemData.plannedPathNode() = new osg::Geode;
    systemData.plannedPathNode()->addDrawable(createTrail(osg::Vec4(1.0f, 0.8f, 0.0f, 1.0f)));
    systemNode->rollingMap()->addChild(systemData.plannedPathNode(), false);
#endif

    systemData.rgbImageNode() = new ImageWindowGeode;
    systemData.rgbImageNode()->init("RGB Image", osg::Vec4(0.0f, 0.0f, 0.1f, 1.0f),
                                    m3DWidget->font());
    m3DWidget->hudGroup()->addChild(systemData.rgbImageNode(), false);

    systemData.depthImageNode() = new ImageWindowGeode;
    systemData.depthImageNode()->init("Depth Image", osg::Vec4(0.0f, 0.0f, 0.1f, 1.0f),
                                      m3DWidget->font());
    m3DWidget->hudGroup()->addChild(systemData.depthImageNode(), false);

    // find available models
    addModels(systemData.models(), systemColor);
    systemViewParams->modelNames();
    for (int i = 0; i < systemData.models().size(); ++i)
    {
        systemViewParams->modelNames().push_back(systemData.models()[i]->getName().c_str());
    }

    systemData.modelNode() = systemData.models().front();
    systemNode->egocentricMap()->addChild(systemData.modelNode());

    connect(systemViewParams.data(), SIGNAL(modelChangedSignal(int,int)),
            this, SLOT(modelChanged(int,int)));
}

void
Pixhawk3DWidget::getPose(UASInterface* uas,
                         MAV_FRAME frame,
                         double& x, double& y, double& z,
                         double& roll, double& pitch, double& yaw,
                         QString& utmZone) const
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
Pixhawk3DWidget::getPose(UASInterface* uas,
                         MAV_FRAME frame,
                         double& x, double& y, double& z,
                         double& roll, double& pitch, double& yaw) const
{
    QString utmZone;
    getPose(uas, frame, x, y, z, roll, pitch, yaw, utmZone);
}

void
Pixhawk3DWidget::getPosition(UASInterface* uas,
                             MAV_FRAME frame,
                             double& x, double& y, double& z,
                             QString& utmZone) const
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
Pixhawk3DWidget::getPosition(UASInterface* uas,
                             MAV_FRAME frame,
                             double& x, double& y, double& z) const
{
    QString utmZone;
    getPosition(uas, frame, x, y, z, utmZone);
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createLocalGrid(void)
{
    osg::ref_ptr<osg::Geode> geode(new osg::Geode());
    osg::ref_ptr<osg::Geometry> fineGeometry(new osg::Geometry());
    osg::ref_ptr<osg::Geometry> coarseGeometry(new osg::Geometry());
    geode->addDrawable(fineGeometry);
    geode->addDrawable(coarseGeometry);

    float radius = 5.0f;
    float resolution = 0.25f;

    osg::ref_ptr<osg::Vec3Array> fineCoords(new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> coarseCoords(new osg::Vec3Array);

    // draw a 10m x 10m grid with 0.25m resolution
    for (float i = -radius; i <= radius; i += resolution)
    {
        if (fabs(i / 1.0f - floor(i / 1.0f)) < 0.01f)
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
    fineStateset->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    fineStateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    fineGeometry->setStateSet(fineStateset);

    osg::ref_ptr<osg::StateSet> coarseStateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> coarseLinewidth(new osg::LineWidth());
    coarseLinewidth->setWidth(1.0f);
    coarseStateset->setAttributeAndModes(coarseLinewidth, osg::StateAttribute::ON);
    coarseStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    coarseStateset->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    coarseStateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    coarseGeometry->setStateSet(coarseStateset);

    return geode;
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createWorldGrid(void)
{
    osg::ref_ptr<osg::Geode> geode(new osg::Geode());
    osg::ref_ptr<osg::Geometry> fineGeometry(new osg::Geometry());
    osg::ref_ptr<osg::Geometry> coarseGeometry(new osg::Geometry());
    osg::ref_ptr<osg::Geometry> axisGeometry(new osg::Geometry());
    geode->addDrawable(fineGeometry);
    geode->addDrawable(coarseGeometry);
    geode->addDrawable(axisGeometry.get());

    float radius = 20.0f;
    float resolution = 1.0f;

    osg::ref_ptr<osg::Vec3Array> fineCoords(new osg::Vec3Array);
    osg::ref_ptr<osg::Vec3Array> coarseCoords(new osg::Vec3Array);

    // draw a 40m x 40m grid with 1.0m resolution
    for (float i = -radius; i <= radius; i += resolution)
    {
        if (fabs(i / 5.0f - floor(i / 5.0f)) < 0.01f)
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
    fineLinewidth->setWidth(0.1f);
    fineStateset->setAttributeAndModes(fineLinewidth, osg::StateAttribute::ON);
    fineStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    fineStateset->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    fineStateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    fineGeometry->setStateSet(fineStateset);

    osg::ref_ptr<osg::StateSet> coarseStateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> coarseLinewidth(new osg::LineWidth());
    coarseLinewidth->setWidth(2.0f);
    coarseStateset->setAttributeAndModes(coarseLinewidth, osg::StateAttribute::ON);
    coarseStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    coarseStateset->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    coarseStateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    coarseGeometry->setStateSet(coarseStateset);

    // add axes
    osg::ref_ptr<osg::Vec3Array> coords(new osg::Vec3Array(6));
    (*coords)[0] = (*coords)[2] = (*coords)[4] =
                                      osg::Vec3(0.0f, 0.0f, 0.0f);
    (*coords)[1] = osg::Vec3(0.0f, 1.0f, 0.0f);
    (*coords)[3] = osg::Vec3(1.0f, 0.0f, 0.0f);
    (*coords)[5] = osg::Vec3(0.0f, 0.0f, -1.0f);

    axisGeometry->setVertexArray(coords);

    osg::Vec4 redColor(1.0f, 0.0f, 0.0f, 0.0f);
    osg::Vec4 greenColor(0.0f, 1.0f, 0.0f, 0.0f);
    osg::Vec4 blueColor(0.0f, 0.0f, 1.0f, 0.0f);

    osg::ref_ptr<osg::Vec4Array> axisColors(new osg::Vec4Array(6));
    (*axisColors)[0] = redColor;
    (*axisColors)[1] = redColor;
    (*axisColors)[2] = greenColor;
    (*axisColors)[3] = greenColor;
    (*axisColors)[4] = blueColor;
    (*axisColors)[5] = blueColor;

    axisGeometry->setColorArray(axisColors);
    axisGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    axisGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 6));

    osg::ref_ptr<osg::StateSet> axisStateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> axisLinewidth(new osg::LineWidth());
    axisLinewidth->setWidth(4.0f);
    axisStateset->setAttributeAndModes(axisLinewidth, osg::StateAttribute::ON);
    axisStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    axisGeometry->setStateSet(axisStateset);

    return geode;
}

osg::ref_ptr<osg::Geometry>
Pixhawk3DWidget::createTrail(const osg::Vec4& color)
{
    osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry());
    geometry->setUseDisplayList(false);

    osg::ref_ptr<osg::Vec3dArray> vertices(new osg::Vec3dArray());
    geometry->setVertexArray(vertices);

    osg::ref_ptr<osg::DrawArrays> drawArrays(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP));
    geometry->addPrimitiveSet(drawArrays);

    osg::ref_ptr<osg::Vec4Array> colorArray(new osg::Vec4Array);
    colorArray->push_back(color);
    geometry->setColorArray(colorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth());
    linewidth->setWidth(1.0f);
    stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geometry->setStateSet(stateset);

    return geometry;
}

osg::ref_ptr<osg::Geometry>
Pixhawk3DWidget::createLink(const QColor& color)
{
    osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry());
    geometry->setUseDisplayList(false);

    osg::ref_ptr<osg::Vec3dArray> vertices(new osg::Vec3dArray());
    geometry->setVertexArray(vertices);

    osg::ref_ptr<osg::DrawArrays> drawArrays(new osg::DrawArrays(osg::PrimitiveSet::LINES));
    geometry->addPrimitiveSet(drawArrays);

    osg::ref_ptr<osg::Vec4Array> colorArray(new osg::Vec4Array);
    colorArray->push_back(osg::Vec4(color.redF(), color.greenF(), color.blueF(), 1.0f));
    geometry->setColorArray(colorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);
    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth());
    linewidth->setWidth(3.0f);
    stateset->setAttributeAndModes(linewidth, osg::StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    geometry->setStateSet(stateset);

    return geometry;
}

osg::ref_ptr<Imagery>
Pixhawk3DWidget::createImagery(void)
{
    return osg::ref_ptr<Imagery>(new Imagery());
}

osg::ref_ptr<osg::Geode>
Pixhawk3DWidget::createPointCloud(void)
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
Pixhawk3DWidget::createTarget(const QColor& color)
{
    osg::ref_ptr<osg::PositionAttitudeTransform> pat =
        new osg::PositionAttitudeTransform;

    pat->setPosition(osg::Vec3d(0.0, 0.0, 0.0));

    osg::ref_ptr<osg::Cone> cone = new osg::Cone(osg::Vec3f(0.0f, 0.0f, 0.0f), 0.2f, 0.6f);
    osg::ref_ptr<osg::ShapeDrawable> coneDrawable = new osg::ShapeDrawable(cone);
    coneDrawable->setColor(osg::Vec4f(color.redF(), color.greenF(), color.blueF(), 1.0f));
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

    mHudBackgroundGeometry = new osg::Geometry;
    mHudBackgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                            0, 4));
    mHudBackgroundGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,
                                            4, 4));
    mHudBackgroundGeometry->setColorArray(hudColors);
    mHudBackgroundGeometry->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    mHudBackgroundGeometry->setUseDisplayList(false);

    mStatusText = new osgText::Text;
    mStatusText->setCharacterSize(11);
    mStatusText->setFont(m3DWidget->font());
    mStatusText->setAxisAlignment(osgText::Text::SCREEN);
    mStatusText->setColor(osg::Vec4(255, 255, 255, 1));

    osg::ref_ptr<osg::Geode> statusGeode = new osg::Geode;
    statusGeode->addDrawable(mHudBackgroundGeometry);
    statusGeode->addDrawable(mStatusText);
    m3DWidget->hudGroup()->addChild(statusGeode);

    mScaleGeode = new HUDScaleGeode;
    mScaleGeode->init(m3DWidget->font());
    m3DWidget->hudGroup()->addChild(mScaleGeode);
}

void
Pixhawk3DWidget::resizeHUD(int width, int height)
{
    int topHUDHeight = 25;
    int bottomHUDHeight = 25;

    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(mHudBackgroundGeometry->getVertexArray());
    if (vertices == NULL || vertices->size() != 8)
    {
        osg::ref_ptr<osg::Vec3Array> newVertices = new osg::Vec3Array(8);
        mHudBackgroundGeometry->setVertexArray(newVertices);

        vertices = static_cast<osg::Vec3Array*>(mHudBackgroundGeometry->getVertexArray());
    }

    (*vertices)[0] = osg::Vec3(0, height, -1);
    (*vertices)[1] = osg::Vec3(width, height, -1);
    (*vertices)[2] = osg::Vec3(width, height - topHUDHeight, -1);
    (*vertices)[3] = osg::Vec3(0, height - topHUDHeight, -1);
    (*vertices)[4] = osg::Vec3(0, 0, -1);
    (*vertices)[5] = osg::Vec3(width, 0, -1);
    (*vertices)[6] = osg::Vec3(width, bottomHUDHeight, -1);
    (*vertices)[7] = osg::Vec3(0, bottomHUDHeight, -1);

    mStatusText->setPosition(osg::Vec3(10, height - 15, -1.5));

    QMutableMapIterator<int, SystemContainer> it(mSystemContainerMap);
    while (it.hasNext())
    {
        it.next();

        SystemContainer& systemData = it.value();

        if (systemData.rgbImageNode().valid() &&
            systemData.depthImageNode().valid())
        {
            int windowWidth = (width - 20) / 2;
            int windowHeight = 3 * windowWidth / 4;
            systemData.rgbImageNode()->setAttributes(10,
                                                     (height - windowHeight) / 2,
                                                     windowWidth,
                                                     windowHeight);
            systemData.depthImageNode()->setAttributes(width / 2,
                                                       (height - windowHeight) / 2,
                                                       windowWidth,
                                                       windowHeight);
        }
    }
}

void
Pixhawk3DWidget::updateHUD(UASInterface* uas, MAV_FRAME frame)
{
    if (!uas) return;
    // display pose of current system
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double roll = 0.0;
    double pitch = 0.0;
    double yaw = 0.0;
    QString utmZone;

    getPose(uas, frame, x, y, z, roll, pitch, yaw, utmZone);

    QPointF cursorPosition =
        m3DWidget->worldCursorPosition(m3DWidget->mouseCursorCoords(), -z);

    std::ostringstream oss;
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss.precision(2);
    oss << "MAV " << uas->getUASID() << ": ";

    if (frame == MAV_FRAME_GLOBAL)
    {
        double latitude, longitude;
        Imagery::UTMtoLL(x, y, utmZone, latitude, longitude);

        double cursorLatitude, cursorLongitude;
        Imagery::UTMtoLL(cursorPosition.x(), cursorPosition.y(),
                         utmZone, cursorLatitude, cursorLongitude);

        oss.precision(6);
        oss << " Lat = " << latitude <<
            " Lon = " << longitude;

        oss.precision(2);
        oss << " Altitude = " << -z <<
            " r = " << roll <<
            " p = " << pitch <<
            " y = " << yaw;

        oss.precision(6);
        oss << " Cursor [" << cursorLatitude <<
            " " << cursorLongitude << "]";
    }
    else if (frame == MAV_FRAME_LOCAL_NED)
    {
        oss << " x = " << x <<
            " y = " << y <<
            " z = " << z <<
            " r = " << roll <<
            " p = " << pitch <<
            " y = " << yaw <<
            " Cursor [" << cursorPosition.x() <<
            " " << cursorPosition.y() << "]";
    }

    mStatusText->setText(oss.str());

    bool darkBackground = true;
    if (frame == MAV_FRAME_GLOBAL &&
        mImageryNode->getImageryType() == Imagery::GOOGLE_MAP)
    {
        darkBackground = false;
    }

    mScaleGeode->update(height(), m3DWidget->cameraParams().fov(),
                        m3DWidget->cameraManipulator()->getDistance(),
                        darkBackground);
}

void
Pixhawk3DWidget::updateTrails(double robotX, double robotY, double robotZ,
                              osg::ref_ptr<osg::Geode>& trailNode,
                              osg::ref_ptr<osg::Group>& orientationNode,
                              QMap<int, QVector<osg::Vec3d> >& trailMap,
                              QMap<int, int>& trailIndexMap)
{
    QMapIterator<int,int> it(trailIndexMap);

    while (it.hasNext())
    {
        it.next();

        // update trail
        osg::Geometry* geometry = trailNode->getDrawable(it.value() * 2)->asGeometry();
        osg::DrawArrays* drawArrays = reinterpret_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0));

        osg::ref_ptr<osg::Vec3Array> vertices(new osg::Vec3Array);

        const QVector<osg::Vec3d>& trail = trailMap.value(it.key());

        vertices->reserve(trail.size());
        for (int i = 0; i < trail.size(); ++i)
        {
            vertices->push_back(osg::Vec3d(trail[i].y() - robotY,
                                           trail[i].x() - robotX,
                                           -(trail[i].z() - robotZ)));
        }

        geometry->setVertexArray(vertices);
        drawArrays->setFirst(0);
        drawArrays->setCount(vertices->size());
        geometry->dirtyBound();

        // update link
        geometry = trailNode->getDrawable(it.value() * 2 + 1)->asGeometry();
        drawArrays = reinterpret_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0));

        vertices = new osg::Vec3Array;

        if (!trail.empty())
        {
            QVector3D p(trail.back().x() - robotX,
                        trail.back().y() - robotY,
                        trail.back().z() - robotZ);

            double length = p.length();
            p.normalize();

            for (double i = 0.1; i < length - 0.1; i += 0.3)
            {
                QVector3D v = p * i;

                vertices->push_back(osg::Vec3d(v.y(), v.x(), -v.z()));
            }
        }
        if (vertices->size() % 2 == 1)
        {
            vertices->pop_back();
        }

        geometry->setVertexArray(vertices);
        drawArrays->setFirst(0);
        drawArrays->setCount(vertices->size());
        geometry->dirtyBound();

        if (!trail.empty())
        {
            osg::PositionAttitudeTransform* pat =
                dynamic_cast<osg::PositionAttitudeTransform*>(orientationNode->getChild(it.value()));
            pat->setPosition(osg::Vec3(trail.back().y() - robotY,
                                       trail.back().x() - robotX,
                                       -(trail.back().z() - robotZ)));
        }
    }
}

void
Pixhawk3DWidget::updateImagery(double originX, double originY,
                               const QString& zone, MAV_FRAME frame)
{
    if (mImageryNode->getImageryType() == Imagery::BLANK_MAP)
    {
        return;
    }

    double viewingRadius = m3DWidget->cameraManipulator()->getDistance() * 10.0;
    if (viewingRadius < 200.0)
    {
        viewingRadius = 200.0;
    }

    double minResolution = 0.25;
    double centerResolution = m3DWidget->cameraManipulator()->getDistance() / 50.0;
    double maxResolution = 1048576.0;

    Imagery::Type imageryType = mImageryNode->getImageryType();
    switch (imageryType)
    {
    case Imagery::GOOGLE_MAP:
        minResolution = 0.25;
        break;
    case Imagery::GOOGLE_SATELLITE:
        minResolution = 0.5;
        break;
    case Imagery::OFFLINE_SATELLITE:
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

    double x = m3DWidget->cameraManipulator()->getCenter().y();
    double y = m3DWidget->cameraManipulator()->getCenter().x();

    double xOffset = 0.0;
    double yOffset = 0.0;

    if (frame == MAV_FRAME_LOCAL_NED)
    {
        xOffset = originX;
        yOffset = originY;
    }

    mImageryNode->draw3D(viewingRadius,
                         resolution,
                         x + xOffset,
                         y + yOffset,
                         -xOffset,
                         -yOffset,
                         zone);

    // prefetch map tiles
    if (resolution / 2.0 >= minResolution)
    {
        mImageryNode->prefetch3D(viewingRadius / 2.0,
                                 resolution / 2.0,
                                 x + xOffset,
                                 y + yOffset,
                                 zone);
    }
    if (resolution * 2.0 <= maxResolution)
    {
        mImageryNode->prefetch3D(viewingRadius * 2.0,
                                 resolution * 2.0,
                                 x + xOffset,
                                 y + yOffset,
                                 zone);
    }

    mImageryNode->update();
}

void
Pixhawk3DWidget::updateTarget(UASInterface* uas, MAV_FRAME frame,
                              double robotX, double robotY, double robotZ,
                              QVector4D& target,
                              osg::ref_ptr<osg::Node>& targetNode)
{
    Q_UNUSED(uas);
    Q_UNUSED(frame);

    osg::PositionAttitudeTransform* pat =
        dynamic_cast<osg::PositionAttitudeTransform*>(targetNode.get());

    pat->setPosition(osg::Vec3d(target.y() - robotY,
                                target.x() - robotX,
                                -(target.z() - robotZ)));
    pat->setAttitude(osg::Quat(target.w() - M_PI_2, osg::Vec3d(1.0f, 0.0f, 0.0f),
                               M_PI_2, osg::Vec3d(0.0f, 1.0f, 0.0f),
                               0.0, osg::Vec3d(0.0f, 0.0f, 1.0f)));
}

void
Pixhawk3DWidget::updateWaypoints(UASInterface* uas, MAV_FRAME frame,
                                 osg::ref_ptr<WaypointGroupNode>& waypointGroupNode)
{
    waypointGroupNode->update(uas, frame);
}

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)

void
Pixhawk3DWidget::updateObstacles(UASInterface* uas, MAV_FRAME frame,
                                 double robotX, double robotY, double robotZ,
                                 osg::ref_ptr<ObstacleGroupNode>& obstacleGroupNode)
{
    if (frame == MAV_FRAME_GLOBAL)
    {
        obstacleGroupNode->clear();
        return;
    }

    qreal receivedTimestamp;
    px::ObstacleList obstacleList = uas->getObstacleList(receivedTimestamp);

    if (QGC::groundTimeSeconds() - receivedTimestamp < kMessageTimeout)
    {
        obstacleGroupNode->update(robotX, robotY, robotZ, obstacleList);
    }
    else
    {
        obstacleGroupNode->clear();
    }
}

void
Pixhawk3DWidget::updatePlannedPath(UASInterface* uas, MAV_FRAME frame,
                                   double robotX, double robotY, double robotZ,
                                   osg::ref_ptr<osg::Geode>& plannedPathNode)
{
    Q_UNUSED(frame);

    qreal receivedTimestamp;
    px::Path path = uas->getPath(receivedTimestamp);

    osg::Geometry* geometry = plannedPathNode->getDrawable(0)->asGeometry();
    osg::DrawArrays* drawArrays = reinterpret_cast<osg::DrawArrays*>(geometry->getPrimitiveSet(0));
    osg::Vec4Array* colorArray = reinterpret_cast<osg::Vec4Array*>(geometry->getColorArray());

    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    osg::ref_ptr<osg::LineWidth> linewidth(new osg::LineWidth());
    linewidth->setWidth(2.0f);
    geometry->getStateSet()->setAttributeAndModes(linewidth, osg::StateAttribute::ON);

    colorArray->clear();

    osg::ref_ptr<osg::Vec3Array> vertices(new osg::Vec3Array);

    if (QGC::groundTimeSeconds() - receivedTimestamp < kMessageTimeout)
    {
        // find path length
        float length = 0.0f;
        for (int i = 0; i < path.waypoints_size() - 1; ++i)
        {
            const px::Waypoint& wp0 = path.waypoints(i);
            const px::Waypoint& wp1 = path.waypoints(i+1);

            length += qgc::hypot3f(wp0.x() - wp1.x(),
                                   wp0.y() - wp1.y(),
                                   wp0.z() - wp1.z());
        }

        // build path
        if (path.waypoints_size() > 0)
        {
            const px::Waypoint& wp0 = path.waypoints(0);

            vertices->push_back(osg::Vec3d(wp0.y() - robotY,
                                           wp0.x() - robotX,
                                           -(wp0.z() - robotZ)));

            float r, g, b;
            qgc::colormap("autumn", 0, r, g, b);
            colorArray->push_back(osg::Vec4d(r, g, b, 1.0f));
        }

        float lengthCurrent = 0.0f;
        for (int i = 0; i < path.waypoints_size() - 1; ++i)
        {
            const px::Waypoint& wp0 = path.waypoints(i);
            const px::Waypoint& wp1 = path.waypoints(i+1);

            lengthCurrent += qgc::hypot3f(wp0.x() - wp1.x(),
                                          wp0.y() - wp1.y(),
                                          wp0.z() - wp1.z());

            vertices->push_back(osg::Vec3d(wp1.y() - robotY,
                                           wp1.x() - robotX,
                                           -(wp1.z() - robotZ)));

            int colorIdx = lengthCurrent / length * 127.0f;

            float r, g, b;
            qgc::colormap("autumn", colorIdx, r, g, b);
            colorArray->push_back(osg::Vec4f(r, g, b, 1.0f));
        }
    }

    geometry->setVertexArray(vertices);
    drawArrays->setFirst(0);
    drawArrays->setCount(vertices->size());
    geometry->dirtyBound();
}

void
Pixhawk3DWidget::updateRGBD(UASInterface* uas, MAV_FRAME frame,
                            osg::ref_ptr<ImageWindowGeode>& rgbImageNode,
                            osg::ref_ptr<ImageWindowGeode>& depthImageNode)
{
    Q_UNUSED(frame);

    qreal receivedTimestamp;
    px::RGBDImage rgbdImage = uas->getRGBDImage(receivedTimestamp);

    if (rgbdImage.rows() > 0 && rgbdImage.cols() > 0 &&
        QGC::groundTimeSeconds() - receivedTimestamp < kMessageTimeout)
    {
        rgbImageNode->image()->setImage(rgbdImage.cols(), rgbdImage.rows(), 1,
                                        GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                        reinterpret_cast<unsigned char *>(&(*(rgbdImage.mutable_imagedata1()))[0]),
                                        osg::Image::NO_DELETE);
        rgbImageNode->image()->dirty();

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

                    float r, g, b;
                    qgc::colormap("jet", idx, r, g, b);
                    pixel[0] = r * 255.0f;
                    pixel[1] = g * 255.0f;
                    pixel[2] = b * 255.0f;
                }

                pixel += 3;
            }
        }

        depthImageNode->image()->setImage(rgbdImage.cols(), rgbdImage.rows(), 1,
                                          GL_RGB, GL_RGB, GL_UNSIGNED_BYTE,
                                          reinterpret_cast<unsigned char *>(coloredDepth.data()),
                                          osg::Image::NO_DELETE);
        depthImageNode->image()->dirty();
    }
}

void
Pixhawk3DWidget::updatePointCloud(UASInterface* uas, MAV_FRAME frame,
                                  double robotX, double robotY, double robotZ,
                                  osg::ref_ptr<osg::Geode>& pointCloudNode,
                                  bool colorPointCloudByDistance)
{
    Q_UNUSED(frame);

    qreal receivedTimestamp;
    px::PointCloudXYZRGB pointCloud = uas->getPointCloud(receivedTimestamp);

    osg::Geometry* geometry = pointCloudNode->getDrawable(0)->asGeometry();
    osg::Vec3Array* vertices = static_cast<osg::Vec3Array*>(geometry->getVertexArray());
    osg::Vec4Array* colors = static_cast<osg::Vec4Array*>(geometry->getColorArray());

    if (QGC::groundTimeSeconds() - receivedTimestamp > kMessageTimeout)
    {
        geometry->removePrimitiveSet(0, geometry->getNumPrimitiveSets());
        return;
    }

    for (int i = 0; i < pointCloud.points_size(); ++i)
    {
        const px::PointCloudXYZRGB_PointXYZRGB& p = pointCloud.points(i);

        double x = p.x() - robotX;
        double y = p.y() - robotY;
        double z = p.z() - robotZ;


        (*vertices)[i].set(y, x, -z);

        if (!colorPointCloudByDistance)
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

            float r, g, b;
            qgc::colormap("jet", colorIndex, r, g, b);

            (*colors)[i].set(r, g, b, 1.0f);
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

#endif

int
Pixhawk3DWidget::findWaypoint(const QPoint& mousePos)
{
    if (!m3DWidget->getSceneData() || !mActiveUAS)
    {
        return -1;
    }

    SystemContainer& systemData = mSystemContainerMap[mActiveUAS->getUASID()];
    osg::ref_ptr<WaypointGroupNode>& waypointGroupNode = systemData.waypointGroupNode();

    osgUtil::LineSegmentIntersector::Intersections intersections;

    QPoint widgetMousePos = m3DWidget->mapFromParent(mousePos);

    if (m3DWidget->computeIntersections(widgetMousePos.x(),
                                        m3DWidget->height() - widgetMousePos.y(),
                                        intersections))
    {
        for (osgUtil::LineSegmentIntersector::Intersections::iterator
             it = intersections.begin(); it != intersections.end(); it++)
        {
            for (uint i = 0 ; i < it->nodePath.size(); ++i)
            {
                osg::Node* node = it->nodePath[i];
                std::string nodeName = node->getName();
                if (nodeName.substr(0, 2).compare("wp") == 0)
                {
                    if (node->getParent(0)->getParent(0) == waypointGroupNode.get())
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
    if (m3DWidget->getSceneData())
    {
        osgUtil::LineSegmentIntersector::Intersections intersections;

        if (m3DWidget->computeIntersections(mouseX, height() - mouseY, intersections))
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

bool
Pixhawk3DWidget::findTerrain(const QPoint& mousePos)
{
    if (!m3DWidget->getSceneData() || !mActiveUAS)
    {
        return -1;
    }

    osgUtil::LineSegmentIntersector::Intersections intersections;

    QPoint widgetMousePos = m3DWidget->mapFromParent(mousePos);

    if (m3DWidget->computeIntersections(widgetMousePos.x(),
                                        m3DWidget->height() - widgetMousePos.y(),
                                        intersections))
    {
        for (osgUtil::LineSegmentIntersector::Intersections::iterator
             it = intersections.begin(); it != intersections.end(); it++)
        {
            for (uint i = 0 ; i < it->nodePath.size(); ++i)
            {
                osg::Node* node = it->nodePath[i];
                std::string nodeName = node->getName();

                if (nodeName.compare("terrain") == 0)
                {
                    return true;
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
    text = QString("Move waypoint %1").arg(QString::number(mSelectedWpIndex));
    menu.addAction(text, this, SLOT(moveWaypointPosition()));

    text = QString("Change heading of waypoint %1").arg(QString::number(mSelectedWpIndex));
    menu.addAction(text, this, SLOT(moveWaypointHeading()));

    text = QString("Change altitude of waypoint %1").arg(QString::number(mSelectedWpIndex));
    menu.addAction(text, this, SLOT(setWaypointAltitude()));

    text = QString("Delete waypoint %1").arg(QString::number(mSelectedWpIndex));
    menu.addAction(text, this, SLOT(deleteWaypoint()));

    menu.addAction("Clear all waypoints", this, SLOT(clearAllWaypoints()));
    menu.exec(cursorPos);
}

void
Pixhawk3DWidget::showTerrainMenu(const QPoint &cursorPos)
{
    QMenu menu;
    menu.addAction("Edit terrain parameters", this, SLOT(showTerrainParamWindow()));
    menu.exec(cursorPos);
}

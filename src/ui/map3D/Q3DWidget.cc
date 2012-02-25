/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of the class Q3DWidget.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "Q3DWidget.h"
#include "QGC.h"

#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#ifdef QGC_OSG_QT_ENABLED
#include <osgQt/QFontImplementation>
#endif
#ifdef Q_OS_MACX
#include <Carbon/Carbon.h>
#endif

Q3DWidget::Q3DWidget(QWidget* parent)
    : QGLWidget(parent)
    , mHandleDeviceEvents(true)
    , mRoot(new osg::Group())
    , mHudGroup(new osg::Switch())
    , mHudProjectionMatrix(new osg::Projection)
    , mFps(30.0f)
{
#ifdef QGC_OSG_QT_ENABLED
    osg::ref_ptr<osgText::Font::FontImplementation> fontImpl;
    fontImpl = new osgQt::QFontImplementation(QFont(":/general/vera.ttf"));
#else
    osg::ref_ptr<osgText::Font::FontImplementation> fontImpl;
    fontImpl = 0;//new osgText::Font::Font("images/Vera.ttf");
#endif
    mFont = new osgText::Font(fontImpl);

    mOsgGW = new osgViewer::GraphicsWindowEmbedded(0, 0, width(), height());

    setThreadingModel(osgViewer::Viewer::CullDrawThreadPerContext);

    setMouseTracking(true);
}

Q3DWidget::~Q3DWidget()
{

}

void
Q3DWidget::init(float fps)
{
    mFps = fps;

    getCamera()->setGraphicsContext(mOsgGW);

    // manually specify near and far clip planes
    getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    setLightingMode(osg::View::SKY_LIGHT);

    setSceneData(mRoot);

    mWorldMap = new osg::Switch;
    mRoot->addChild(mWorldMap);

    // set up HUD
    mRoot->addChild(createHUD());

    // set up camera control
    mCameraManipulator = new GCManipulator();
    setCameraManipulator(mCameraManipulator);
    mCameraManipulator->setMinZoomRange(mCameraParams.minZoomRange());
    mCameraManipulator->setDistance(mCameraParams.minZoomRange() * 2.0);

    connect(&mTimer, SIGNAL(timeout()), this, SLOT(redraw()));
    // DO NOT START TIMER IN INITIALIZATION! IT IS STARTED IN THE SHOW EVENT
}

void
Q3DWidget::setCameraParams(float minZoomRange, float cameraFov,
                           float minClipRange, float maxClipRange)
{
    mCameraParams.minZoomRange() = minZoomRange;
    mCameraParams.fov() = cameraFov;
    mCameraParams.minClipRange() = minClipRange;
    mCameraParams.maxClipRange() = maxClipRange;
}

void
Q3DWidget::moveCamera(double dx, double dy, double dz)
{
    mCameraManipulator->move(dx, dy, dz);
}

void
Q3DWidget::recenterCamera(double x, double y, double z)
{
    mCameraManipulator->setCenter(osg::Vec3d(x, y, z));
}

void
Q3DWidget::rotateCamera(double roll, double pitch, double yaw)
{
    osg::Quat q(-yaw, osg::Vec3d(0.0f, 0.0f, 1.0f),
                pitch, osg::Vec3d(1.0f, 0.0f, 0.0f),
                roll, osg::Vec3d(0.0f, 1.0f, 0.0f));

    mCameraManipulator->setRotation(q);
}

void
Q3DWidget::setCameraDistance(double distance)
{
    mCameraManipulator->setDistance(distance);
}

void
Q3DWidget::setDisplayMode3D(void)
{
    double aspect = static_cast<double>(width())
                    / static_cast<double>(height());

    getCamera()->setViewport(new osg::Viewport(0, 0, width(), height()));
    getCamera()->setProjectionMatrixAsPerspective(mCameraParams.fov(),
                                                  aspect,
                                                  mCameraParams.minClipRange(),
                                                  mCameraParams.maxClipRange());
}

osg::ref_ptr<osg::Switch>&
Q3DWidget::hudGroup(void)
{
    return mHudGroup;
}

QPoint
Q3DWidget::mouseCursorCoords(void)
{
    return mapFromGlobal(cursor().pos());
}

QPointF
Q3DWidget::worldCursorPosition(const QPoint& cursorPos, double worldZ) const
{
    osgUtil::LineSegmentIntersector::Intersections intersections;

    // normalize cursor position to value between -1 and 1
    double x = -1.0f + static_cast<double>(2 * cursorPos.x())
               / static_cast<double>(width());
    double y = -1.0f + static_cast<double>(2 * (height() - cursorPos.y()))
               / static_cast<double>(height());

    // compute matrix which transforms screen coordinates to world coordinates
    osg::Matrixd m = getCamera()->getViewMatrix()
                     * getCamera()->getProjectionMatrix();
    osg::Matrixd invM = osg::Matrixd::inverse(m);

    osg::Vec3d nearPoint = osg::Vec3d(x, y, -1.0) * invM;
    osg::Vec3d farPoint = osg::Vec3d(x, y, 1.0) * invM;

    osg::ref_ptr<osg::LineSegment> line =
        new osg::LineSegment(nearPoint, farPoint);

    osg::Plane p(osg::Vec3d(0.0, 0.0, 1.0), osg::Vec3d(0.0, 0.0, worldZ));

    osg::Vec3d projectedPoint;
    planeLineIntersection(p.asVec4(), *line, projectedPoint);

    return QPointF(projectedPoint.y(), projectedPoint.x());
}

CameraParams&
Q3DWidget::cameraParams(void)
{
    return mCameraParams;
}

osg::ref_ptr<GCManipulator>&
Q3DWidget::cameraManipulator(void)
{
    return mCameraManipulator;
}

osg::ref_ptr<osgText::Font>&
Q3DWidget::font(void)
{
    return mFont;
}

osg::ref_ptr<osg::Switch>&
Q3DWidget::worldMap(void)
{
    return mWorldMap;
}

osg::ref_ptr<SystemGroupNode>&
Q3DWidget::systemGroup(int systemId)
{
    if (!mSystemGroups.contains(systemId))
    {
        osg::ref_ptr<SystemGroupNode> newSystem = new SystemGroupNode;
        mRoot->addChild(newSystem);

        mSystemGroups.insert(systemId, newSystem);
    }

    return mSystemGroups[systemId];
}

bool&
Q3DWidget::handleDeviceEvents(void)
{
    return mHandleDeviceEvents;
}

void
Q3DWidget::handleKeyPressEvent(QKeyEvent* event)
{
    if (event->isAccepted())
    {
        return;
    }

    if (event->text().isEmpty())
    {
        mOsgGW->getEventQueue()->keyPress(convertKey(event->key()));
    }
    else
    {
        mOsgGW->getEventQueue()->keyPress(
            static_cast<osgGA::GUIEventAdapter::KeySymbol>(
                *(event->text().toAscii().data())));
    }
}

void
Q3DWidget::handleKeyReleaseEvent(QKeyEvent* event)
{
    if (event->isAccepted())
    {
        return;
    }

    if (event->text().isEmpty())
    {
        mOsgGW->getEventQueue()->keyRelease(convertKey(event->key()));
    }
    else
    {
        mOsgGW->getEventQueue()->keyRelease(
            static_cast<osgGA::GUIEventAdapter::KeySymbol>(
                *(event->text().toAscii().data())));
    }
}

void
Q3DWidget::handleMousePressEvent(QMouseEvent* event)
{
    if (event->isAccepted())
    {
        return;
    }

    int button = 0;
    switch (event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MidButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    case Qt::NoButton:
        button = 0;
        break;
    default:
        {}
    }
    mOsgGW->getEventQueue()->mouseButtonPress(event->x(), event->y(), button);
}

void
Q3DWidget::handleMouseReleaseEvent(QMouseEvent* event)
{
    if (event->isAccepted())
    {
        return;
    }

    int button = 0;
    switch (event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;
    case Qt::MidButton:
        button = 2;
        break;
    case Qt::RightButton:
        button = 3;
        break;
    case Qt::NoButton:
        button = 0;
        break;
    default:
        {}
    }
    mOsgGW->getEventQueue()->mouseButtonRelease(event->x(), event->y(), button);
}

void
Q3DWidget::handleMouseMoveEvent(QMouseEvent* event)
{
    if (event->isAccepted())
    {
        return;
    }

    mOsgGW->getEventQueue()->mouseMotion(event->x(), event->y());
}

void
Q3DWidget::handleWheelEvent(QWheelEvent* event)
{
    if (event->isAccepted())
    {
        return;
    }

    mOsgGW->getEventQueue()->mouseScroll((event->delta() > 0) ?
                                         osgGA::GUIEventAdapter::SCROLL_UP :
                                         osgGA::GUIEventAdapter::SCROLL_DOWN);
}

void
Q3DWidget::redraw(void)
{
    emit update();
#if (QGC_EVENTLOOP_DEBUG)
    qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif
    updateGL();
}

void
Q3DWidget::resizeGL(int width, int height)
{
    mHudProjectionMatrix->setMatrix(osg::Matrix::ortho(0.0, width,
                                    0.0, height,
                                    -10.0, 10.0));

    mOsgGW->getEventQueue()->windowResize(0, 0, width, height);
    mOsgGW->resized(0 , 0, width, height);

    emit sizeChanged(width, height);
}

void
Q3DWidget::keyPressEvent(QKeyEvent* event)
{
    QWidget::keyPressEvent(event);

    if (mHandleDeviceEvents)
    {
        handleKeyPressEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void
Q3DWidget::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);

    if (mHandleDeviceEvents)
    {
        handleKeyReleaseEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void
Q3DWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    if (mHandleDeviceEvents)
    {
        handleMousePressEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void
Q3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);

    if (mHandleDeviceEvents)
    {
        handleMouseReleaseEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void
Q3DWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);

    if (mHandleDeviceEvents)
    {
        handleMouseMoveEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void
Q3DWidget::wheelEvent(QWheelEvent* event)
{
    QWidget::wheelEvent(event);

    if (mHandleDeviceEvents)
    {
        handleWheelEvent(event);
    }
    else
    {
        event->ignore();
    }
}

void
Q3DWidget::showEvent(QShowEvent* event)
{
    // React only to internal (pre/post-display)
    // events
    Q_UNUSED(event)
    mTimer.start(static_cast<int>(floorf(1000.0f / mFps)));
}

void
Q3DWidget::hideEvent(QHideEvent* event)
{
    // React only to internal (pre/post-display)
    // events
    Q_UNUSED(event)
    mTimer.stop();
}

osg::ref_ptr<osg::Node>
Q3DWidget::createHUD(void)
{
    mHudProjectionMatrix->setMatrix(osg::Matrix::ortho(0.0, width(),
                                    0.0, height(),
                                    -10.0, 10.0));

    osg::ref_ptr<osg::MatrixTransform> hudModelViewMatrix(
        new osg::MatrixTransform);
    hudModelViewMatrix->setMatrix(osg::Matrix::identity());
    hudModelViewMatrix->setReferenceFrame(osg::Transform::ABSOLUTE_RF);

    mHudProjectionMatrix->addChild(hudModelViewMatrix);
    hudModelViewMatrix->addChild(mHudGroup);

    osg::ref_ptr<osg::StateSet> hudStateSet(new osg::StateSet);
    mHudGroup->setStateSet(hudStateSet);
    hudStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    hudStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    hudStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    hudStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    hudStateSet->setRenderBinDetails(11, "RenderBin");

    return mHudProjectionMatrix;
}

void
Q3DWidget::paintGL(void)
{
    setDisplayMode3D();

    getCamera()->setClearColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
    getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    frame();
}

osgGA::GUIEventAdapter::KeySymbol
Q3DWidget::convertKey(int key) const
{
    switch (key)
    {
    case Qt::Key_Space :
        return osgGA::GUIEventAdapter::KEY_Space;
    case Qt::Key_Backspace :
        return osgGA::GUIEventAdapter::KEY_BackSpace;
    case Qt::Key_Tab :
        return osgGA::GUIEventAdapter::KEY_Tab;
    case Qt::Key_Clear :
        return osgGA::GUIEventAdapter::KEY_Clear;
    case Qt::Key_Return :
        return osgGA::GUIEventAdapter::KEY_Return;
    case Qt::Key_Enter :
        return osgGA::GUIEventAdapter::KEY_KP_Enter;
    case Qt::Key_Pause :
        return osgGA::GUIEventAdapter::KEY_Pause;
    case Qt::Key_ScrollLock :
        return osgGA::GUIEventAdapter::KEY_Scroll_Lock;
    case Qt::Key_SysReq :
        return osgGA::GUIEventAdapter::KEY_Sys_Req;
    case Qt::Key_Escape :
        return osgGA::GUIEventAdapter::KEY_Escape;
    case Qt::Key_Delete :
        return osgGA::GUIEventAdapter::KEY_Delete;
    case Qt::Key_Home :
        return osgGA::GUIEventAdapter::KEY_Home;
    case Qt::Key_Left :
        return osgGA::GUIEventAdapter::KEY_Left;
    case Qt::Key_Up :
        return osgGA::GUIEventAdapter::KEY_Up;
    case Qt::Key_Right :
        return osgGA::GUIEventAdapter::KEY_Right;
    case Qt::Key_Down :
        return osgGA::GUIEventAdapter::KEY_Down;
    case Qt::Key_PageUp :
        return osgGA::GUIEventAdapter::KEY_Page_Up;
    case Qt::Key_PageDown :
        return osgGA::GUIEventAdapter::KEY_Page_Down;
    case Qt::Key_End :
        return osgGA::GUIEventAdapter::KEY_End;
    case Qt::Key_Select :
        return osgGA::GUIEventAdapter::KEY_Select;
    case Qt::Key_Print :
        return osgGA::GUIEventAdapter::KEY_Print;
    case Qt::Key_Execute :
        return osgGA::GUIEventAdapter::KEY_Execute;
    case Qt::Key_Insert :
        return osgGA::GUIEventAdapter::KEY_Insert;
    case Qt::Key_Menu :
        return osgGA::GUIEventAdapter::KEY_Menu;
    case Qt::Key_Cancel :
        return osgGA::GUIEventAdapter::KEY_Cancel;
    case Qt::Key_Help :
        return osgGA::GUIEventAdapter::KEY_Help;
    case Qt::Key_Mode_switch :
        return osgGA::GUIEventAdapter::KEY_Mode_switch;
    case Qt::Key_NumLock :
        return osgGA::GUIEventAdapter::KEY_Num_Lock;
    case Qt::Key_Equal :
        return osgGA::GUIEventAdapter::KEY_KP_Equal;
    case Qt::Key_Asterisk :
        return osgGA::GUIEventAdapter::KEY_KP_Multiply;
    case Qt::Key_Plus :
        return osgGA::GUIEventAdapter::KEY_KP_Add;
    case Qt::Key_Minus :
        return osgGA::GUIEventAdapter::KEY_KP_Subtract;
    case Qt::Key_Comma :
        return osgGA::GUIEventAdapter::KEY_KP_Decimal;
    case Qt::Key_Slash :
        return osgGA::GUIEventAdapter::KEY_KP_Divide;
    case Qt::Key_0 :
        return osgGA::GUIEventAdapter::KEY_KP_0;
    case Qt::Key_1 :
        return osgGA::GUIEventAdapter::KEY_KP_1;
    case Qt::Key_2 :
        return osgGA::GUIEventAdapter::KEY_KP_2;
    case Qt::Key_3 :
        return osgGA::GUIEventAdapter::KEY_KP_3;
    case Qt::Key_4 :
        return osgGA::GUIEventAdapter::KEY_KP_4;
    case Qt::Key_5 :
        return osgGA::GUIEventAdapter::KEY_KP_5;
    case Qt::Key_6 :
        return osgGA::GUIEventAdapter::KEY_KP_6;
    case Qt::Key_7 :
        return osgGA::GUIEventAdapter::KEY_KP_7;
    case Qt::Key_8 :
        return osgGA::GUIEventAdapter::KEY_KP_8;
    case Qt::Key_9 :
        return osgGA::GUIEventAdapter::KEY_KP_9;
    case Qt::Key_F1 :
        return osgGA::GUIEventAdapter::KEY_F1;
    case Qt::Key_F2 :
        return osgGA::GUIEventAdapter::KEY_F2;
    case Qt::Key_F3 :
        return osgGA::GUIEventAdapter::KEY_F3;
    case Qt::Key_F4 :
        return osgGA::GUIEventAdapter::KEY_F4;
    case Qt::Key_F5 :
        return osgGA::GUIEventAdapter::KEY_F5;
    case Qt::Key_F6 :
        return osgGA::GUIEventAdapter::KEY_F6;
    case Qt::Key_F7 :
        return osgGA::GUIEventAdapter::KEY_F7;
    case Qt::Key_F8 :
        return osgGA::GUIEventAdapter::KEY_F8;
    case Qt::Key_F9 :
        return osgGA::GUIEventAdapter::KEY_F9;
    case Qt::Key_F10 :
        return osgGA::GUIEventAdapter::KEY_F10;
    case Qt::Key_F11 :
        return osgGA::GUIEventAdapter::KEY_F11;
    case Qt::Key_F12 :
        return osgGA::GUIEventAdapter::KEY_F12;
    case Qt::Key_F13 :
        return osgGA::GUIEventAdapter::KEY_F13;
    case Qt::Key_F14 :
        return osgGA::GUIEventAdapter::KEY_F14;
    case Qt::Key_F15 :
        return osgGA::GUIEventAdapter::KEY_F15;
    case Qt::Key_F16 :
        return osgGA::GUIEventAdapter::KEY_F16;
    case Qt::Key_F17 :
        return osgGA::GUIEventAdapter::KEY_F17;
    case Qt::Key_F18 :
        return osgGA::GUIEventAdapter::KEY_F18;
    case Qt::Key_F19 :
        return osgGA::GUIEventAdapter::KEY_F19;
    case Qt::Key_F20 :
        return osgGA::GUIEventAdapter::KEY_F20;
    case Qt::Key_F21 :
        return osgGA::GUIEventAdapter::KEY_F21;
    case Qt::Key_F22 :
        return osgGA::GUIEventAdapter::KEY_F22;
    case Qt::Key_F23 :
        return osgGA::GUIEventAdapter::KEY_F23;
    case Qt::Key_F24 :
        return osgGA::GUIEventAdapter::KEY_F24;
    case Qt::Key_F25 :
        return osgGA::GUIEventAdapter::KEY_F25;
    case Qt::Key_F26 :
        return osgGA::GUIEventAdapter::KEY_F26;
    case Qt::Key_F27 :
        return osgGA::GUIEventAdapter::KEY_F27;
    case Qt::Key_F28 :
        return osgGA::GUIEventAdapter::KEY_F28;
    case Qt::Key_F29 :
        return osgGA::GUIEventAdapter::KEY_F29;
    case Qt::Key_F30 :
        return osgGA::GUIEventAdapter::KEY_F30;
    case Qt::Key_F31 :
        return osgGA::GUIEventAdapter::KEY_F31;
    case Qt::Key_F32 :
        return osgGA::GUIEventAdapter::KEY_F32;
    case Qt::Key_F33 :
        return osgGA::GUIEventAdapter::KEY_F33;
    case Qt::Key_F34 :
        return osgGA::GUIEventAdapter::KEY_F34;
    case Qt::Key_F35 :
        return osgGA::GUIEventAdapter::KEY_F35;
    case Qt::Key_Shift :
        return osgGA::GUIEventAdapter::KEY_Shift_L;
//    case Qt::Key_Shift_R : return osgGA::GUIEventAdapter::KEY_Shift_R;
    case Qt::Key_Control :
        return osgGA::GUIEventAdapter::KEY_Control_L;
//    case Qt::Key_Control_R : return osgGA::GUIEventAdapter::KEY_Control_R;
    case Qt::Key_CapsLock :
        return osgGA::GUIEventAdapter::KEY_Caps_Lock;
    case Qt::Key_Meta :
        return osgGA::GUIEventAdapter::KEY_Meta_L;
//    case Qt::Key_Meta_R: return osgGA::GUIEventAdapter::KEY_Meta_R;
    case Qt::Key_Alt :
        return osgGA::GUIEventAdapter::KEY_Alt_L;
//    case Qt::Key_Alt_R : return osgGA::GUIEventAdapter::KEY_Alt_R;
    case Qt::Key_Super_L :
        return osgGA::GUIEventAdapter::KEY_Super_L;
    case Qt::Key_Super_R :
        return osgGA::GUIEventAdapter::KEY_Super_R;
    case Qt::Key_Hyper_L :
        return osgGA::GUIEventAdapter::KEY_Hyper_L;
    case Qt::Key_Hyper_R :
        return osgGA::GUIEventAdapter::KEY_Hyper_R;
    default:
        return static_cast<osgGA::GUIEventAdapter::KeySymbol>(key);
    }
}

bool
Q3DWidget::planeLineIntersection(const osg::Vec4d& plane,
                                 const osg::LineSegment& line,
                                 osg::Vec3d& isect) const
{
    osg::Vec3d lineStart = line.start();
    osg::Vec3d lineEnd = line.end();

    const double deltaX = lineEnd.x() - lineStart.x();
    const double deltaY = lineEnd.y() - lineStart.y();
    const double deltaZ = lineEnd.z() - lineStart.z();

    const double denominator = plane[0] * deltaX
                               + plane[1] * deltaY
                               + plane[2] * deltaZ;
    if (!denominator)
    {
        return false;
    }

    const double C = (plane[0] * lineStart.x()
                      + plane[1] * lineStart.y()
                      + plane[2] * lineStart.z()
                      + plane[3]) / denominator;

    isect.x() = lineStart.x() - deltaX * C;
    isect.y() = lineStart.y() - deltaY * C;
    isect.z() = lineStart.z() - deltaZ * C;

    return true;
}

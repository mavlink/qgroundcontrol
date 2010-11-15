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

#ifndef Q3DWIDGET_H
#define Q3DWIDGET_H

#include <QtOpenGL>

#include <osg/LineSegment>
#include <osg/PositionAttitudeTransform>
#include <osgViewer/Viewer>

enum MouseState
{
    MOUSE_STATE_UP = 0,
    MOUSE_STATE_DOWN = 1
};

typedef void (*DisplayFunc)(void *);
typedef void (*KeyboardFunc)(char, void *);
typedef void (*MouseFunc)(Qt::MouseButton, MouseState, int, int, void *);
typedef void (*MotionFunc)(int, int, void *);

class Q3DWidget : public QGLWidget, public osgViewer::Viewer
{
    Q_OBJECT

public:
    Q3DWidget(QWidget* parent = 0);
    virtual ~Q3DWidget();

    void init(float fps);
    void setCameraParams(float minZoomRange, float cameraFov,
                         float minClipRange,
                         float maxClipRange);

    void forceRedraw(void);
    void recenter(void);
    void setDisplayMode3D(void);

    void setDisplayFunc(DisplayFunc func, void* clientData);
    void setKeyboardFunc(KeyboardFunc func, void* clientData);
    void setMouseFunc(MouseFunc func, void* clientData);
    void setMotionFunc(MotionFunc func, void* clientData);
    void addTimerFunc(uint msecs, void(*func)(void *),
                      void* clientData);

    std::pair<double,double> getGlobalCursorPosition(int32_t mouseX,
                                                     int32_t mouseY,
                                                     double z);

protected slots:
    void redraw(void);
    void userTimer(void);

protected:
    osg::ref_ptr<osg::Geode> createRobot(void);
    osg::ref_ptr<osg::Node> createHUD(void);

    int getMouseX(void);
    int getMouseY(void);
    int getLastMouseX(void);
    int getLastMouseY(void);

    osgViewer::GraphicsWindow* getGraphicsWindow(void);
    const osgViewer::GraphicsWindow* getGraphicsWindow(void) const;

    virtual void resizeGL(int width, int height);
    virtual void paintGL(void);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

    float r2d(float angle);
    float d2r(float angle);
    osgGA::GUIEventAdapter::KeySymbol convertKey(int key) const;
    bool getPlaneLineIntersection(const osg::Vec4d& plane,
                                  const osg::LineSegment& line,
                                  osg::Vec3d& isect);

    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Switch> allocentricMap;
    osg::ref_ptr<osg::Switch> rollingMap;
    osg::ref_ptr<osg::Switch> egocentricMap;
    osg::ref_ptr<osg::PositionAttitudeTransform> robotPosition;
    osg::ref_ptr<osg::PositionAttitudeTransform> robotAttitude;

    osg::ref_ptr<osg::Geode> hudGeode;
    osg::ref_ptr<osg::Projection> hudProjectionMatrix;

    DisplayFunc userDisplayFunc;
    KeyboardFunc userKeyboardFunc;
    MouseFunc userMouseFunc;
    MotionFunc userMotionFunc;
    void (*userTimerFunc)(void *);

    void* userDisplayFuncData;
    void* userKeyboardFuncData;
    void* userMouseFuncData;
    void* userMotionFuncData;
    void* userTimerFuncData;

    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgGW;

    QTimer timer;

    struct CameraParams
    {
        float minZoomRange;
        float cameraFov;
        float minClipRange;
        float maxClipRange;
    };

    CameraParams cameraParams;

    int lastMouseX;
    int lastMouseY;

    bool _forceRedraw;
};

#endif // Q3DWIDGET_H

#ifndef Q3DWIDGET_H_
#define Q3DWIDGET_H_

#include <boost/scoped_ptr.hpp>
#include <inttypes.h>
#include <string>
#include <QtOpenGL>
#include <QtGui>

enum CameraState
{
    IDLE = 0,
    ROTATING = 1,
    MOVING = 2,
    ZOOMING = 3
};

struct CameraPose
{
    CameraState state;
    float pan, tilt, distance;
    float xOffset, yOffset, zOffset;
    float xOffset2D, yOffset2D, rotation2D, zoom, warpX, warpY;
};

struct CameraParams
{
    float zoomSensitivity;
    float rotateSensitivity;
    float moveSensitivity;

    float minZoomRange;
    float cameraFov;
    float minClipRange;
    float maxClipRange;

    float zoomSensitivity2D;
    float rotateSensitivity2D;
    float moveSensitivity2D;
};

enum MouseState
{
    MOUSE_STATE_UP = 0,
    MOUSE_STATE_DOWN = 1
};

typedef void (*DisplayFunc)(void *);
typedef void (*KeyboardFunc)(char, void *);
typedef void (*MouseFunc)(Qt::MouseButton, MouseState, int32_t, int32_t, void *);
typedef void (*MotionFunc)(int32_t, int32_t, void *);

class Q3DWidget: public QGLWidget
{
    Q_OBJECT

public:
    explicit Q3DWidget(QWidget* parent);
    ~Q3DWidget();

    void initialize(int32_t windowX, int32_t windowY,
                    int32_t windowWidth, int32_t windowHeight, float fps);

    void setCameraParams(float zoomSensitivity, float rotateSensitivity,
                         float moveSensitivity, float minZoomRange,
                         float cameraFov, float minClipRange,
                         float maxClipRange);

    void setCameraLimit(bool onoff);
    void setCameraLock(bool onoff);

    void set2DCameraParams(float zoomSensitivity,
                           float rotateSensitivity,
                           float moveSensitivity);

    void set3D(bool onoff);
    bool is3D(void) const;

    void setInitialCameraPos(float pan, float tilt, float range,
                             float xOffset, float yOffset, float zOffset);
    void setInitial2DCameraPos(float xOffset, float yOffset,
                               float rotation, float zoom);
    void setCameraPose(const CameraPose& cameraPose);
    CameraPose getCameraPose(void) const;

    void setDisplayFunc(DisplayFunc func, void* clientData);
    void setKeyboardFunc(KeyboardFunc func, void* clientData);
    void setMouseFunc(MouseFunc func, void* clientData);
    void setMotionFunc(MotionFunc func, void* clientData);
    void addTimerFunc(uint32_t msecs, void(*func)(void *),
                      void* clientData);

    void forceRedraw(void);

    void set2DWarping(float warpX, float warpY);

    void recenter(void);
    void recenter2D(void);

    void set2DRotation(bool onoff);

    void setDisplayMode2D(void);

    std::pair<float,float> getPositionIn3DMode(int32_t mouseX,
                                               int32_t mouseY);

    std::pair<float,float> getPositionIn2DMode(int32_t mouseX,
                                               int32_t mouseY);

    int32_t getWindowWidth(void);
    int32_t getWindowHeight(void);
    int32_t getLastMouseX(void);
    int32_t getLastMouseY(void);
    int32_t getMouseX(void);
    int32_t getMouseY(void);

private Q_SLOTS:
    void userTimer(void);

protected:
    void rotateCamera(float dx, float dy);
    void zoomCamera(float dy);
    void moveCamera(float dx, float dy);
    void rotateCamera2D(float dx);
    void zoomCamera2D(float dx);
    void moveCamera2D(float dx, float dy);

    void switchTo3DMode(void);
    void setDisplayMode3D(void);

    float r2d(float angle);
    float d2r(float angle);

private:
    // QGLWidget events
    void initializeGL(void);
    void paintGL(void);
    void resizeGL(int32_t width, int32_t height);

    // Qt events
    void keyPressEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent *wheel);
    void timerEvent(QTimerEvent* event);
    void closeEvent(QCloseEvent* event);

    DisplayFunc userDisplayFunc;
    KeyboardFunc userKeyboardFunc;
    MouseFunc userMouseFunc;
    MotionFunc userMotionFunc;

    void* userDisplayFuncData;
    void* userKeyboardFuncData;
    void* userMouseFuncData;
    void* userMotionFuncData;

    int32_t windowWidth, windowHeight;
    float requestedFps;
    CameraPose cameraPose;
    int32_t lastMouseX, lastMouseY;

    bool _is3D;

    bool _forceRedraw;
    bool allow2DRotation;
    bool limitCamera;
    bool lockCamera;

    CameraParams cameraParams;

    QBasicTimer timer;

    void (*timerFunc)(void *);
    void* timerFuncData;
};

#endif

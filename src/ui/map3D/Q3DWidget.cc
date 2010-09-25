#include "Q3DWidget.h"

#include <cmath>

//#include <GL/gl.h>
//#include <GL/glu.h>

static const float KEY_ROTATE_AMOUNT = 5.0f;
static const float KEY_MOVE_AMOUNT   = 10.0f;
static const float KEY_ZOOM_AMOUNT   = 5.0f;

Q3DWidget::Q3DWidget(QWidget* parent)
  : QGLWidget(QGLFormat(QGL::Rgba | QGL::DoubleBuffer | QGL:: DepthBuffer |
                        QGL::StencilBuffer), parent)
  , userDisplayFunc(NULL)
  , userKeyboardFunc(NULL)
  , userMouseFunc(NULL)
  , userMotionFunc(NULL)
  , userDisplayFuncData(NULL)
  , userKeyboardFuncData(NULL)
  , userMouseFuncData(NULL)
  , userMotionFuncData(NULL)
  , windowWidth(0)
  , windowHeight(0)
  , requestedFps(0.0f)
  , lastMouseX(0)
  , lastMouseY(0)
  , _is3D(true)
  , _forceRedraw(false)
  , allow2DRotation(true)
  , limitCamera(false)
  , lockCamera(false)
  , timerFunc(NULL)
  , timerFuncData(NULL)
{
    cameraPose.state = IDLE;
    cameraPose.pan = 0.0f;
    cameraPose.tilt = 180.0f;
    cameraPose.distance = 10.0f;
    cameraPose.xOffset = 0.0f;
    cameraPose.yOffset = 0.0f;
    cameraPose.zOffset = 0.0f;

    cameraPose.xOffset2D = 0.0f;
    cameraPose.yOffset2D = 0.0f;
    cameraPose.rotation2D = 0.0f;
    cameraPose.zoom = 1.0f;
    cameraPose.warpX = 1.0f;
    cameraPose.warpY = 1.0f;

    cameraParams.zoomSensitivity = 0.05f;
    cameraParams.rotateSensitivity = 0.5f;
    cameraParams.moveSensitivity = 0.001f;
    cameraParams.minZoomRange = 0.5f;
    cameraParams.cameraFov = 30.0f;
    cameraParams.minClipRange = 1.0f;
    cameraParams.maxClipRange = 400.0f;
    cameraParams.zoomSensitivity2D = 0.02f;
    cameraParams.rotateSensitivity2D = 0.005f;
    cameraParams.moveSensitivity2D = 1.0f;
}

Q3DWidget::~Q3DWidget()
{

}

void
Q3DWidget::initialize(int32_t windowX, int32_t windowY,
                      int32_t windowWidth, int32_t windowHeight, float fps)
{
    this->windowWidth = windowWidth;
    this->windowHeight = windowHeight;

    requestedFps = fps;

    resize(windowWidth, windowHeight);
    move(windowX, windowY);

    timer.start(static_cast<int>(floorf(1000.0f / requestedFps)), this);

    _is3D = true;
}

void
Q3DWidget::setCameraParams(float zoomSensitivity, float rotateSensitivity,
							 float moveSensitivity, float minZoomRange,
							 float cameraFov, float minClipRange,
							 float maxClipRange)
{
    cameraParams.zoomSensitivity = zoomSensitivity;
    cameraParams.rotateSensitivity = rotateSensitivity;
    cameraParams.moveSensitivity = moveSensitivity;
    cameraParams.minZoomRange = minZoomRange;
    cameraParams.cameraFov = cameraFov;
    cameraParams.minClipRange = minClipRange;
    cameraParams.maxClipRange = maxClipRange;

    limitCamera = true;
    _forceRedraw = true;
}

void
Q3DWidget::setCameraLimit(bool onoff)
{
    limitCamera = onoff;
}

void
Q3DWidget::setCameraLock(bool onoff)
{
    lockCamera = onoff;
}

void
Q3DWidget::set2DCameraParams(float zoomSensitivity2D,
                             float rotateSensitivity2D,
                             float moveSensitivity2D)
{
    cameraParams.zoomSensitivity2D = zoomSensitivity2D;
    cameraParams.rotateSensitivity2D = rotateSensitivity2D;
    cameraParams.moveSensitivity2D = moveSensitivity2D;
}

void
Q3DWidget::set3D(bool onoff)
{
    _is3D = onoff;
}

bool
Q3DWidget::is3D(void) const
{
    return _is3D;
}

void
Q3DWidget::setInitialCameraPos(float pan, float tilt, float range,
								 float xOffset, float yOffset, float zOffset)
{
    cameraPose.pan = pan;
    cameraPose.tilt = tilt;
    cameraPose.distance = range;
    cameraPose.xOffset = xOffset;
    cameraPose.yOffset = yOffset;
    cameraPose.zOffset = zOffset;
}

void
Q3DWidget::setInitial2DCameraPos(float xOffset, float yOffset,
								   float rotation, float zoom)
{
    cameraPose.xOffset2D = xOffset;
    cameraPose.yOffset2D = yOffset;
    cameraPose.rotation2D = rotation;
    cameraPose.zoom = zoom;
}

void
Q3DWidget::setCameraPose(const CameraPose& cameraPose)
{
    this->cameraPose = cameraPose;
}

CameraPose
Q3DWidget::getCameraPose(void) const
{
    return cameraPose;
}

void
Q3DWidget::setDisplayFunc(DisplayFunc func, void* clientData)
{
    userDisplayFunc = func;
    userDisplayFuncData = clientData;
}

void
Q3DWidget::setKeyboardFunc(KeyboardFunc func, void* clientData)
{
    userKeyboardFunc = func;
    userKeyboardFuncData = clientData;
}

void
Q3DWidget::setMouseFunc(MouseFunc func, void* clientData)
{
    userMouseFunc = func;
    userMouseFuncData = clientData;
}

void
Q3DWidget::setMotionFunc(MotionFunc func, void* clientData)
{
    userMotionFunc = func;
    userMotionFuncData = clientData;
}

void
Q3DWidget::addTimerFunc(uint32_t msecs, void(*func)(void *),
                          void* clientData)
{
    timerFunc = func;
    timerFuncData = clientData;

    QTimer::singleShot(msecs, this, SLOT(userTimer()));
}

void
Q3DWidget::userTimer(void)
{
    if (timerFunc)
    {
            timerFunc(timerFuncData);
    }
}

void
Q3DWidget::forceRedraw(void)
{
    _forceRedraw = true;
}

void
Q3DWidget::set2DWarping(float warpX, float warpY)
{
    cameraPose.warpX = warpX;
    cameraPose.warpY = warpY;
}

void
Q3DWidget::recenter(void)
{
    cameraPose.xOffset = 0.0f;
    cameraPose.yOffset = 0.0f;
    cameraPose.zOffset = 0.0f;
}

void
Q3DWidget::recenter2D(void)
{
    cameraPose.xOffset2D = 0.0f;
    cameraPose.yOffset2D = 0.0f;
}

void
Q3DWidget::set2DRotation(bool onoff)
{
    allow2DRotation = onoff;
}

void
Q3DWidget::setDisplayMode2D(void)
{
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLfloat>(getWindowWidth()),
            0.0, static_cast<GLfloat>(getWindowHeight()),
            -10.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

std::pair<float,float>
Q3DWidget::getPositionIn3DMode(int32_t mouseX, int32_t mouseY)
{
    float cx = windowWidth / 2.0f;
    float cy = windowHeight / 2.0f;
    float pan = d2r(-90.0f - cameraPose.pan);
    float tilt = d2r(90.0f - cameraPose.tilt);
    float d = cameraPose.distance;
    float f = cy / tanf(d2r(cameraParams.cameraFov / 2.0f));

    float px = (mouseX - cx) * cosf(tilt) * d / (cosf(tilt) * f + sinf(tilt)
                            * mouseY - sinf(tilt) * cy);
    float py = -(mouseY - cy) * d / (cosf(tilt) * f + sinf(tilt) * mouseY
                            - sinf(tilt) * cy);

    std::pair<float,float> sceneCoords;
    sceneCoords.first = px * cosf(pan) + py * sinf(pan) + cameraPose.xOffset;
    sceneCoords.second = -px * sinf(pan) + py * cosf(pan) + cameraPose.yOffset;

    return sceneCoords;
}

std::pair<float,float>
Q3DWidget::getPositionIn2DMode(int32_t mouseX, int32_t mouseY)
{
    float dx = (mouseX - windowWidth / 2.0f) / cameraPose.zoom;
    float dy = (windowHeight / 2.0f - mouseY) / cameraPose.zoom;
    float ctheta = cosf(-cameraPose.rotation2D);
    float stheta = sinf(-cameraPose.rotation2D);

    std::pair<float,float> coords;
    coords.first = cameraPose.xOffset2D + ctheta * dx - stheta * dy;
    coords.second = cameraPose.yOffset2D + stheta * dx + ctheta * dy;

    return coords;
}

int
Q3DWidget::getWindowWidth(void)
{
    return windowWidth;
}

int
Q3DWidget::getWindowHeight(void)
{
    return windowHeight;
}

int
Q3DWidget::getLastMouseX(void)
{
    return lastMouseX;
}

int
Q3DWidget::getLastMouseY(void)
{
    return lastMouseY;
}

int
Q3DWidget::getMouseX(void)
{
    return mapFromGlobal(cursor().pos()).x();
}

int
Q3DWidget::getMouseY(void)
{
    return mapFromGlobal(cursor().pos()).y();
}


void
Q3DWidget::rotateCamera(float dx, float dy)
{
    if (!lockCamera)
    {
	cameraPose.pan += dx * cameraParams.rotateSensitivity;
	cameraPose.tilt += dy * cameraParams.rotateSensitivity;
	if (limitCamera)
	{
            if (cameraPose.tilt < 180.5f)
            {
                cameraPose.tilt = 180.5f;
            }
            else if (cameraPose.tilt > 269.5f)
            {
                cameraPose.tilt = 269.5f;
            }
	}
    }
}

void
Q3DWidget::zoomCamera(float dy)
{
    cameraPose.distance -=
            dy * cameraParams.zoomSensitivity * cameraPose.distance;
    if (cameraPose.distance < cameraParams.minZoomRange)
    {
        cameraPose.distance = cameraParams.minZoomRange;
    }
}

void
Q3DWidget::moveCamera(float dx, float dy)
{
    cameraPose.xOffset +=
            -dy * cosf(d2r(cameraPose.pan)) * cameraParams.moveSensitivity
            * cameraPose.distance;
    cameraPose.yOffset +=
            -dy * sinf(d2r(cameraPose.pan)) * cameraParams.moveSensitivity
            * cameraPose.distance;
    cameraPose.xOffset += dx * cosf(d2r(cameraPose.pan - 90.0f))
            * cameraParams.moveSensitivity * cameraPose.distance;
    cameraPose.yOffset += dx * sinf(d2r(cameraPose.pan - 90.0f))
            * cameraParams.moveSensitivity * cameraPose.distance;
}

void
Q3DWidget::rotateCamera2D(float dx)
{
    if (allow2DRotation)
    {
        cameraPose.rotation2D += dx * cameraParams.rotateSensitivity2D;
    }
}

void
Q3DWidget::zoomCamera2D(float dx)
{
    cameraPose.zoom += dx * cameraParams.zoomSensitivity2D * cameraPose.zoom;
    if (cameraPose.zoom > 1e7f)
    {
        cameraPose.zoom = 1e7f;
    }
    if (cameraPose.zoom < 1e-7f)
    {
        cameraPose.zoom = 1e-7f;
    }
}

void
Q3DWidget::moveCamera2D(float dx, float dy)
{
    float scaledX = dx / cameraPose.zoom;
    float scaledY = dy / cameraPose.zoom;

    cameraPose.xOffset2D -= (scaledX * cosf(-cameraPose.rotation2D)
            + scaledY * sinf(-cameraPose.rotation2D)) / cameraPose.warpX
            * cameraParams.moveSensitivity2D;
    cameraPose.yOffset2D -= (scaledX * sinf(-cameraPose.rotation2D)
            - scaledY * cosf(-cameraPose.rotation2D)) / cameraPose.warpY
            * cameraParams.moveSensitivity2D;
}

void Q3DWidget::switchTo3DMode(void)
{
    // setup camera view
    float cpan = d2r(cameraPose.pan);
    float ctilt = d2r(cameraPose.tilt);
    float cameraX = cameraPose.distance * cosf(cpan) * cosf(ctilt);
    float cameraY = cameraPose.distance * sinf(cpan) * cosf(ctilt);
    float cameraZ = cameraPose.distance * sinf(ctilt);
    setDisplayMode3D();
    glViewport(0, 0, static_cast<GLsizei>(windowWidth),
               static_cast<GLsizei>(windowHeight));
    gluLookAt(cameraX + cameraPose.xOffset, cameraY + cameraPose.yOffset,
              cameraZ + cameraPose.zOffset, cameraPose.xOffset,
              cameraPose.yOffset, cameraPose.zOffset, 0.0, 0.0, 1.0);
}

void
Q3DWidget::setDisplayMode3D()
{
    float aspect = static_cast<float>(getWindowWidth()) /
                   static_cast<float>(getWindowHeight());

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(cameraParams.cameraFov, aspect,
                   cameraParams.minClipRange, cameraParams.maxClipRange);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(-1.0f, -1.0f, 1.0f);
}

float
Q3DWidget::r2d(float angle)
{
    return angle * 57.295779513082320876f;
}

float
Q3DWidget::d2r(float angle)
{
    return angle * 0.0174532925199432957692f;
}

void
Q3DWidget::initializeGL(void)
{
    float lightAmbient[] = {0.0f, 0.0f, 0.0f, 0.0f};
    float lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float lightPosition[] = {0.0f, 0.0f, 100.0f, 0.0f};

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    // TODO: Added these, please check
    // Fix for some platforms, e.g. windows
    #ifndef GL_MULTISAMPLE
    glEnable(0x809D);
    #else
    glEnable(GL_MULTISAMPLE);
    #endif
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void
Q3DWidget::paintGL(void)
{
    if (_is3D)
    {
        // setup camera view
        switchTo3DMode();
    }
    else
    {
        setDisplayMode2D();
        // do camera control
        glTranslatef(static_cast<float>(windowWidth) / 2.0f,
                                 static_cast<float>(windowHeight) / 2.0f,
                                 0.0f);
        glScalef(cameraPose.zoom, cameraPose.zoom, 1.0f);
        glRotatef(r2d(cameraPose.rotation2D), 0.0f, 0.0f, 1.0f);
        glScalef(cameraPose.warpX, cameraPose.warpY, 1.0f);
        glTranslatef(-cameraPose.xOffset2D, -cameraPose.yOffset2D, 0.0f);
    }

    // turn on smooth lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(1.0f);

    if (userDisplayFunc)
    {
        userDisplayFunc(userDisplayFuncData);
    }
    glFlush();
}

void
Q3DWidget::resizeGL(int32_t width, int32_t height)
{
    glViewport(0, 0, width, height);

    windowWidth = width;
    windowHeight = height;

    if (_is3D)
    {
        setDisplayMode3D();
    }
    else
    {
        setDisplayMode2D();
    }
}

void
Q3DWidget::keyPressEvent(QKeyEvent* event)
{
	float dx = 0.0f, dy = 0.0f;

	Qt::KeyboardModifiers modifiers = event->modifiers();
	if (_is3D)
	{
		if (modifiers & Qt::ControlModifier)
		{
			switch (event->key())
			{
			case Qt::Key_Left:
				dx = -KEY_ROTATE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Right:
				dx = KEY_ROTATE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Up:
				dx = 0.0f;
				dy = KEY_ROTATE_AMOUNT;
				break;
			case Qt::Key_Down:
				dx = 0.0f;
				dy = -KEY_ROTATE_AMOUNT;
				break;
			default:
				QWidget::keyPressEvent(event);
			}
			if (dx != 0.0f || dy != 0.0f)
			{
				rotateCamera(dx, dy);
			}
		}
		else if (modifiers & Qt::AltModifier)
		{
			switch (event->key())
			{
			case Qt::Key_Up:
				dy = KEY_ZOOM_AMOUNT;
				break;
			case Qt::Key_Down:
				dy = -KEY_ZOOM_AMOUNT;
				break;
			default:
				QWidget::keyPressEvent(event);
			}
			if (dy != 0.0f)
			{
				zoomCamera(dy);
			}
		}
		else
		{
			switch (event->key())
			{
			case Qt::Key_Left:
				dx = KEY_MOVE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Right:
				dx = -KEY_MOVE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Up:
				dx = 0.0f;
				dy = -KEY_MOVE_AMOUNT;
				break;
			case Qt::Key_Down:
				dx = 0.0f;
				dy = KEY_MOVE_AMOUNT;
				break;
			default:
				QWidget::keyPressEvent(event);
			}
			if (dx != 0.0f || dy != 0.0f)
			{
				moveCamera(dx, dy);
			}
		}
	}
	else {
		if (modifiers & Qt::ControlModifier)
		{
			switch (event->key())
			{
			case Qt::Key_Left:
				dx = KEY_ROTATE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Right:
				dx = -KEY_ROTATE_AMOUNT;
				dy = 0.0f;
				break;
			default:
				QWidget::keyPressEvent(event);
			}
			if (dx != 0.0f)
			{
				rotateCamera2D(dx);
			}
		}
		else if (modifiers & Qt::AltModifier)
		{
			switch (event->key())
			{
			case Qt::Key_Up:
				dy = KEY_ZOOM_AMOUNT;
				break;
			case Qt::Key_Down:
				dy = -KEY_ZOOM_AMOUNT;
				break;
			default:
				QWidget::keyPressEvent(event);
			}
			if (dy != 0.0f)
			{
				zoomCamera2D(dy);
			}
		}
		else {
			switch (event->key())
			{
			case Qt::Key_Left:
				dx = KEY_MOVE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Right:
				dx = -KEY_MOVE_AMOUNT;
				dy = 0.0f;
				break;
			case Qt::Key_Up:
				dx = 0.0f;
				dy = KEY_MOVE_AMOUNT;
				break;
			case Qt::Key_Down:
				dx = 0.0f;
				dy = -KEY_MOVE_AMOUNT;
				break;
			default:
				QWidget::keyPressEvent(event);
			}
			if (dx != 0.0f || dy != 0.0f)
			{
				moveCamera2D(dx, dy);
			}
		}
	}

	_forceRedraw = true;

	if (userKeyboardFunc)
	{
		if (event->text().isEmpty())
		{
			userKeyboardFunc(0, userKeyboardFuncData);
		}
		else
		{
			userKeyboardFunc(event->text().at(0).toAscii(),
							 userKeyboardFuncData);
		}
	}
}

void
Q3DWidget::mousePressEvent(QMouseEvent* event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers();

	if (!(modifiers & (Qt::ControlModifier | Qt::AltModifier)))
	{
		lastMouseX = event->x();
		lastMouseY = event->y();
		if (event->button() == Qt::LeftButton)
		{
			cameraPose.state = ROTATING;
		}
		else if (event->button() == Qt::MidButton)
		{
			cameraPose.state = MOVING;
		}
		else if (event->button() == Qt::RightButton)
		{
			cameraPose.state = ZOOMING;
		}
	}

	_forceRedraw = true;

	if (userMouseFunc)
	{
		userMouseFunc(event->button(), MOUSE_STATE_DOWN, event->x(), event->y(),
					  userMouseFuncData);
	}
}

void
Q3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers();

	if (!(modifiers & (Qt::ControlModifier | Qt::AltModifier)))
	{
		cameraPose.state = IDLE;
	}

	_forceRedraw = true;

	if (userMouseFunc)
	{
		userMouseFunc(event->button(), MOUSE_STATE_UP, event->x(), event->y(),
					  userMouseFuncData);
	}
}

void
Q3DWidget::mouseMoveEvent(QMouseEvent* event)
{
	int32_t dx = event->x() - lastMouseX;
	int32_t dy = event->y() - lastMouseY;

	if (_is3D)
	{
		if (cameraPose.state == ROTATING)
		{
			rotateCamera(static_cast<float>(dx), static_cast<float>(dy));
		}
		else if (cameraPose.state == MOVING)
		{
			moveCamera(static_cast<float>(dx), static_cast<float>(dy));
		}
		else if (cameraPose.state == ZOOMING)
		{
			zoomCamera(static_cast<float>(dy));
		}
	}
	else
	{
		if (cameraPose.state == ROTATING)
		{
			if (event->x() > windowWidth / 2)
			{
				dy *= -1;
			}

			rotateCamera2D(static_cast<float>(dx));
		}
		else if (cameraPose.state == MOVING)
		{
			moveCamera2D(static_cast<float>(dx), static_cast<float>(dy));
		}
		else if (cameraPose.state == ZOOMING)
		{
			zoomCamera2D(static_cast<float>(dy));
		}
	}

	lastMouseX = event->x();
	lastMouseY = event->y();
	_forceRedraw = true;

	if (userMotionFunc)
	{
		userMotionFunc(event->x(), event->y(), userMotionFuncData);
	}
}

void
Q3DWidget::wheelEvent(QWheelEvent* event)
{
	if (_is3D)
	{
		zoomCamera(static_cast<float>(event->delta()) / 40.0f);
	}
	else
	{
		zoomCamera2D(static_cast<float>(event->delta()) / 40.0f);
	}

	_forceRedraw = true;
}

void
Q3DWidget::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == timer.timerId())
	{
		if (_forceRedraw)
		{
			updateGL();
			_forceRedraw = false;
		}
	}
	else
	{
		QObject::timerEvent(event);
	}
}

void
Q3DWidget::closeEvent(QCloseEvent *)
{
    // exit application
}

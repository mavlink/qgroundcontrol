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
 *   @brief Definition of the class QMap3DWidget.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "QMap3DWidget.h"

#include <FTGL/ftgl.h>
#include <GL/glut.h>
#include <QCheckBox>
#include <sys/time.h>

#include "CheetahModel.h"
#include "UASManager.h"
#include "UASInterface.h"

QMap3DWidget::QMap3DWidget(QWidget* parent)
     : Q3DWidget(parent)
     , uas(NULL)
     , lastRedrawTime(0.0)
     , displayGrid(false)
     , displayTrail(false)
     , lockCamera(true)
     , updateLastUnlockedPose(true)
     , displayTarget(false)
{
    setFocusPolicy(Qt::StrongFocus);

    initialize(10, 10, 1000, 900, 10.0f);
    setCameraParams(0.05f, 0.5f, 0.01f, 0.5f, 30.0f, 0.01f, 400.0f);

    int32_t argc = 0;
    glutInit(&argc, NULL);

    setDisplayFunc(display, this);
    setMouseFunc(mouse, this);
    addTimerFunc(100, timer, this);

    buildLayout();

    font.reset(new FTTextureFont("images/Vera.ttf"));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this, SLOT(setActiveUAS(UASInterface*)));
}

QMap3DWidget::~QMap3DWidget()
{

}

void
QMap3DWidget::buildLayout(void)
{
    QCheckBox* gridCheckBox = new QCheckBox(this);
    gridCheckBox->setText("Grid");
    gridCheckBox->setChecked(displayGrid);

    QCheckBox* trailCheckBox = new QCheckBox(this);
    trailCheckBox->setText("Trail");
    trailCheckBox->setChecked(displayTrail);

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
    layout->addWidget(recenterButton, 1, 2);
    layout->addWidget(lockCameraCheckBox, 1, 3);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 50);
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
QMap3DWidget::display(void* clientData)
{
    QMap3DWidget* map3d = reinterpret_cast<QMap3DWidget *>(clientData);
    map3d->displayHandler();
}

void
QMap3DWidget::displayHandler(void)
{
    if (cheetahModel.data() == 0)
    {
        cheetahModel.reset(new CheetahModel);
        cheetahModel->init(1.0f, 1.0f, 1.0f);
    }

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

    if (updateLastUnlockedPose)
    {
        lastUnlockedPose.x = robotX;
        lastUnlockedPose.y = robotY;
        lastUnlockedPose.z = robotZ;

        camOffset.x = 0.0f;
        camOffset.y = 0.0f;
        camOffset.z = 0.0f;

        updateLastUnlockedPose = false;
    }

    if (!lockCamera)
    {
        camOffset.x = robotX - lastUnlockedPose.x;
        camOffset.y = robotY - lastUnlockedPose.y;
        camOffset.z = robotZ - lastUnlockedPose.z;
    }

    // turn on smooth lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // clear window
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(camOffset.x, camOffset.y, camOffset.z);

    // draw Cheetah model
    drawPlatform(robotRoll, robotPitch, robotYaw);

    if (displayGrid)
    {
        drawGrid();
    }

    if (displayTrail)
    {
        drawTrail(robotX, robotY, robotZ);
    }

    if (displayTarget)
    {
        drawTarget(robotX, robotY, robotZ);
    }

    glPopMatrix();

    // switch to 2D
    setDisplayMode2D();

    // display pose information
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 45.0f);
    glVertex2f(getWindowWidth(), 45.0f);
    glVertex2f(getWindowWidth(), 0.0f);
    glEnd();

    char buffer[7][255];

    sprintf(buffer[0], "x = %.2f", robotX);
    sprintf(buffer[1], "y = %.2f", robotY);
    sprintf(buffer[2], "z = %.2f", robotZ);
    sprintf(buffer[3], "r = %.2f", robotRoll);
    sprintf(buffer[4], "p = %.2f", robotPitch);
    sprintf(buffer[5], "y = %.2f", robotYaw);

    std::pair<float,float> mouseWorldCoords =
            getPositionIn3DMode(getMouseX(), getMouseY());
    sprintf(buffer[6], "Cursor [%.2f %.2f]",
            mouseWorldCoords.first + robotX, mouseWorldCoords.second + robotY);

    font->FaceSize(10);
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();

    glTranslatef(0.0f, 30.0f, 0.0f);
    for (int32_t i = 0; i < 7; ++i)
    {
        glTranslatef(60.0f, 0.0f, 0.0f);
        font->Render(buffer[i]);
    }
    glPopMatrix();
}

void
QMap3DWidget::mouse(Qt::MouseButton button, MouseState state,
                    int32_t x, int32_t y, void* clientData)
{
    QMap3DWidget* map3d = reinterpret_cast<QMap3DWidget *>(clientData);
    map3d->mouseHandler(button, state, x, y);
}

void
QMap3DWidget::mouseHandler(Qt::MouseButton button, MouseState state,
                           int32_t x, int32_t y)
{
    if (button == Qt::RightButton && state == MOUSE_STATE_DOWN)
    {
        QMenu menu(this);
        QAction* targetAction = menu.addAction("Mark as Target");
        connect(targetAction, SIGNAL(triggered()), this, SLOT(markTarget()));
        menu.exec(mapToGlobal(QPoint(x, y)));
    }
}

void
QMap3DWidget::timer(void* clientData)
{
    QMap3DWidget* map3d = reinterpret_cast<QMap3DWidget *>(clientData);
    map3d->timerHandler();
}

void
QMap3DWidget::timerHandler(void)
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
QMap3DWidget::getTime(void) const
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
void QMap3DWidget::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL && this->uas != uas)
    {
        // Disconnect any previously connected active MAV
        //disconnect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    }

    this->uas = uas;
}

void
QMap3DWidget::markTarget(void)
{
    std::pair<float,float> mouseWorldCoords =
            getPositionIn3DMode(getLastMouseX(), getLastMouseY());

    float robotX = 0.0f, robotY = 0.0f, robotZ = 0.0f;
    if (uas != NULL)
    {
        robotX = uas->getLocalX();
        robotY = uas->getLocalY();
        robotZ = uas->getLocalZ();
    }

    targetPosition.x = mouseWorldCoords.first + robotX;
    targetPosition.y = mouseWorldCoords.second + robotY;
    targetPosition.z = robotZ;

    displayTarget = true;

    uas->setTargetPosition(targetPosition.x, targetPosition.y,
                           targetPosition.z, 0.0f);
}

void
QMap3DWidget::showGrid(int32_t state)
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
QMap3DWidget::showTrail(int32_t state)
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
QMap3DWidget::recenterCamera(void)
{
    updateLastUnlockedPose = true;

    recenter();
}

void
QMap3DWidget::toggleLockCamera(int32_t state)
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

void
QMap3DWidget::drawPlatform(float roll, float pitch, float yaw)
{
    glPushMatrix();

    glRotatef(yaw, 0.0f, 0.0f, 1.0f);
    glRotatef(pitch, 0.0f, 1.0f, 0.0f);
    glRotatef(roll, 1.0f, 0.0f, 0.0f);

    glLineWidth(3.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.3f, 0.0f, 0.0f);
    glEnd();

    cheetahModel->draw();

    glPopMatrix();
}

void
QMap3DWidget::drawGrid(void)
{
    float radius = 10.0f;
    float resolution = 0.25f;

    glPushMatrix();

    // draw a 20m x 20m grid with 0.25m resolution
    glColor3f(0.5f, 0.5f, 0.5f);
    for (float i = -radius; i <= radius; i += resolution)
    {
        if (fabsf(i - roundf(i)) < 0.01f)
        {
            glLineWidth(2.0f);
        }
        else
        {
            glLineWidth(0.25f);
        }

        glBegin(GL_LINES);
        glVertex3f(i, -radius, 0.0f);
        glVertex3f(i, radius, 0.0f);
        glVertex3f(-radius, i, 0.0f);
        glVertex3f(radius, i, 0.0f);
        glEnd();
    }

    glPopMatrix();
}

void
QMap3DWidget::drawTrail(float x, float y, float z)
{
    bool addToTrail = false;
    if (trail.size() > 0)
    {
        if (fabsf(x - trail[trail.size() - 1].x) > 0.01f ||
            fabsf(y - trail[trail.size() - 1].y) > 0.01f ||
            fabsf(z - trail[trail.size() - 1].z) > 0.01f)
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
        Pose3D p = {x, y, z};
        if (trail.size() == trail.capacity())
        {
            memcpy(trail.data(), trail.data() + 1,
                   (trail.size() - 1) * sizeof(Pose3D));
            trail[trail.size() - 1] = p;
        }
        else
        {
            trail.append(p);
        }
    }

    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    for (int32_t i = 0; i < trail.size(); ++i)
    {
        glVertex3f(trail[i].x - x, trail[i].y - y, trail[i].z - z);
    }
    glEnd();
}

void
QMap3DWidget::drawTarget(float x, float y, float z)
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

    glPushMatrix();
    glTranslatef(targetPosition.x - x, targetPosition.y - y, 0.0f);
    glColor3f(0.0f, 0.7f, 1.0f);
    glLineWidth(1.0f);
    glutWireSphere(radius, 10, 10);

    if (expand)
    {
        radius += 0.02;
    }
    else
    {
        radius -= 0.02;
    }

    glPopMatrix();
}

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

#include <QCheckBox>
#include <sys/time.h>

//#include "QGCGlut.h"
#include "CheetahModel.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "QGC.h"

QMap3DWidget::QMap3DWidget(QWidget* parent)
     : Q3DWidget(parent)
     , uas(NULL)
     , lastRedrawTime(0.0)
     , displayGrid(true)
     , displayImagery(false)
     , displayTrail(false)
     , lockCamera(true)
     , updateLastUnlockedPose(true)
     , displayTarget(false)
     , displayWaypoints(true)
     , imagery(0)
{
    setFocusPolicy(Qt::StrongFocus);

    initialize(10, 10, 1000, 900, 15.0f);
    setCameraParams(0.05f, 0.5f, 0.01f, 0.5f, 30.0f, 0.01f, 1000000.0f);

    setDisplayFunc(display, this);
    setMouseFunc(mouse, this);
    addTimerFunc(100, timer, this);

    buildLayout();

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

    QCheckBox* waypointsCheckBox = new QCheckBox(this);
    waypointsCheckBox->setText("Waypoints");
    waypointsCheckBox->setChecked(displayWaypoints);

    QLabel* imageryLabel = new QLabel(this);
    imageryLabel->setText("Imagery");

    imageryComboBox = new QComboBox(this);
    imageryComboBox->addItem("None");
    imageryComboBox->addItem("Map (Google)");
    imageryComboBox->addItem("Satellite (Google)");
    imageryComboBox->addItem("Satellite (Swisstopo)");

    QPushButton* recenterButton = new QPushButton(this);
    recenterButton->setText("Recenter Camera");

    QCheckBox* lockCameraCheckBox = new QCheckBox(this);
    lockCameraCheckBox->setText("Lock Camera");
    lockCameraCheckBox->setChecked(lockCamera);

    //positionLabel = new QLabel(this);
    //positionLabel->setText(tr("Waiting for first position update.. "));

    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(gridCheckBox, 1, 0);
    layout->addWidget(trailCheckBox, 1, 1);
    layout->addWidget(waypointsCheckBox, 1, 2);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 3);
    layout->addWidget(imageryLabel, 1, 4);
    layout->addWidget(imageryComboBox, 1, 5);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 6);
    layout->addWidget(recenterButton, 1, 7);
    layout->addWidget(lockCameraCheckBox, 1, 8);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    layout->setColumnStretch(3, 50);
    layout->setColumnStretch(6, 50);
    setLayout(layout);

    connect(gridCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showGrid(int)));
    connect(trailCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showTrail(int)));
    connect(imageryComboBox, SIGNAL(currentIndexChanged(const QString &)),
            this, SLOT(showImagery(const QString &)));
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



//void QMap3DWidget::paintEvent(QPaintEvent *event)
//{
//    Q_UNUSED(event);
//}

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

    if (updateLastUnlockedPose && uas != NULL)
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
    makeCurrent();
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
        drawGrid(-camOffset.x, -camOffset.y, robotZ);
    }

    if (displayTrail)
    {
        drawTrail(robotX, robotY, robotZ);
    }

    if (displayTarget)
    {
        drawTarget(robotX, robotY, robotZ);
    }

    if (displayWaypoints)
    {
        drawWaypoints();
    }

    if (displayImagery)
    {
        drawImagery(robotX, robotY, robotZ, "32T", true);
    }

    glPopMatrix();

    // switch to 2D
    setDisplayMode2D();

    drawLegend();

    // display pose information
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.0f, 30.0f);
    glVertex2f(getWindowWidth(), 30.0f);
    glVertex2f(getWindowWidth(), 0.0f);
    glEnd();
    glColor4f(0.1f, 0.1f, 0.1f, 1.0f);
    glBegin(GL_POLYGON);
    glVertex2f(0.0f, getWindowHeight());
    glVertex2f(0.0f, getWindowHeight() - 25.0f);
    glVertex2f(getWindowWidth(), getWindowHeight() - 25.0f);
    glVertex2f(getWindowWidth(), getWindowHeight());
    glEnd();

    glFlush();

    std::pair<float,float> mouseWorldCoords =
            getPositionIn3DMode(getMouseX(), getMouseY());

    // QT QPAINTER OPENGL PAINTING

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    paintText(QString("x = %1 y = %2 z = %3 r = %4 p = %5 y = %6 Cursor [%7 %8]").arg(robotX, 0, 'f', 2).arg(robotY, 0, 'f', 2).arg(robotZ, 0, 'f', 2).arg(robotRoll, 0, 'f', 2).arg(robotPitch, 0, 'f', 2).arg(robotYaw, 0, 'f', 2).arg( mouseWorldCoords.first + robotX, 0, 'f', 2).arg( mouseWorldCoords.second + robotY, 0, 'f', 2),
              QColor(255, 255, 255),
              11,
              5,
              5,
              &painter);
    painter.end();
}

void QMap3DWidget::drawWaypoints(void) const
{
    if (uas)
    {
        const QVector<Waypoint*>& list = uas->getWaypointManager().getWaypointList();
        QColor color;

        QPointF lastWaypoint;

        for (int i = 0; i < list.size(); i++)
        {
            QPointF in(list.at(i)->getX(), list.at(i)->getY());
            // Transform from world to body coordinates
            //in = metricWorldToBody(in);

            // DRAW WAYPOINT
            float waypointRadius = 0.1f;// = vwidth / 20.0f * 2.0f;


            // Select color based on if this is the current waypoint
            if (list.at(i)->getCurrent())
            {
                color = QGC::colorCyan;//uas->getColor();

            }
            else
            {
                color = uas->getColor();

            }

            //float radius = (waypointSize/2.0f) * 0.8 * (1/sqrt(2.0f));
            // Draw yaw
            // Draw sphere





            static double radius = 0.2;

            glPushMatrix();
            glTranslatef(in.x() - uas->getLocalX(), in.y() - uas->getLocalY(), 0.0f);
            glColor3f(1.0f, 0.3f, 0.3f);
            glLineWidth(1.0f);

            wireSphere(radius, 10, 10);

            glPopMatrix();

            // DRAW CONNECTING LINE
            // Draw line from last waypoint to this one
            if (!lastWaypoint.isNull())
            {
                // OpenGL line
            }
            lastWaypoint = in;
        }
    }
}

void
QMap3DWidget::drawLegend(void)
{
    // draw marker outlines
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(20.0f, 60.0f);
    glVertex2f(20.0f, 80.0f);
    glVertex2f(20.0f, 70.0f);
    glVertex2f(100.0f, 70.0f);
    glVertex2f(100.0f, 60.0f);
    glVertex2f(100.0f, 80.0f);
    glEnd();

    // draw markers
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(20.0f, 60.0f);
    glVertex2f(20.0f, 80.0f);
    glVertex2f(20.0f, 70.0f);
    glVertex2f(100.0f, 70.0f);
    glVertex2f(100.0f, 60.0f);
    glVertex2f(100.0f, 80.0f);
    glEnd();

    float f = windowHeight / 2.0f / tanf(d2r(cameraParams.cameraFov / 2.0f));
    float dist = cameraPose.distance / f * 80.0f;

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    QColor rgb(255, 255, 255);
    if (imageryComboBox->currentText().compare("Map (Google)") == 0)
    {
        rgb.setRgb(0, 0, 0);
    }

    paintText(QString("%1 m").arg(dist, 0, 'f', 2),
              rgb,
              10,
              25,
              getWindowHeight() - 65,
              &painter);
    painter.end();
}

void
QMap3DWidget::paintText(QString text, QColor color, float fontSize,
                        float refX, float refY, QPainter* painter) const
{
    QPen prevPen = painter->pen();

    float pPositionX = refX;
    float pPositionY = refY;

    QFont font("Bitstream Vera Sans");
    // Enforce minimum font size of 5 pixels
    int fSize = qMax(5, (int)(fontSize));
    font.setPixelSize(fSize);

    QFontMetrics metrics = QFontMetrics(font);
    int border = qMax(4, metrics.leading());
    QRect rect = metrics.boundingRect(0, 0, width() - 2*border, int(height()*0.125),
                                      Qt::AlignLeft | Qt::TextWordWrap, text);
    painter->setPen(color);
    painter->setFont(font);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->drawText(pPositionX, pPositionY,
                      rect.width(), rect.height(),
                      Qt::AlignCenter | Qt::TextWordWrap, text);
    painter->setPen(prevPen);
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
        QAction* targetAction = menu.addAction(tr("Mark as Target"));
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
    if (imagery.isNull())
    {
        imagery.reset(new Imagery);
    }

    double timeLapsed = getTime() - lastRedrawTime;
    if (timeLapsed > 0.1)
    {
        imagery->update();
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

    if (uas)
    {
        uas->setTargetPosition(targetPosition.x, targetPosition.y,
                               targetPosition.z, 0.0f);
    }
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
QMap3DWidget::showImagery(const QString& text)
{
    if (text.compare("None") == 0)
    {
        displayImagery = false;
    }
    else
    {
        if (text.compare("Map (Google)") == 0)
        {
            imagery->setImageryType(Imagery::GOOGLE_MAP);
        }
        else if (text.compare("Satellite (Google)") == 0)
        {
            imagery->setImageryType(Imagery::GOOGLE_SATELLITE);
        }
        else if (text.compare("Satellite (Swisstopo)") == 0)
        {
            imagery->setImageryType(Imagery::SWISSTOPO_SATELLITE);
        }
        displayImagery = true;
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
QMap3DWidget::drawPlatform(float roll, float pitch, float yaw) const
{
    glPushMatrix();

    glRotatef(yaw * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
    glRotatef(pitch * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
    glRotatef(roll * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);

    glLineWidth(3.0f);

    // X AXIS
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.3f, 0.0f, 0.0f);
    glEnd();

    // Y AXIS
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.15f, 0.0f);
    glEnd();

    // Z AXIS
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.15f);
    glEnd();

    cheetahModel->draw();

    glPopMatrix();
}

void
QMap3DWidget::drawGrid(float x, float y, float z) const
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
        glVertex3f(x + i, y - radius, -z);
        glVertex3f(x + i, y + radius, -z);
        glVertex3f(x - radius, y + i, -z);
        glVertex3f(x + radius, y + i, -z);
        glEnd();
    }

    glPopMatrix();
}

void
QMap3DWidget::drawImagery(double originX, double originY, double originZ,
                          const QString& zone, bool prefetch) const
{
    glPushMatrix();
    glEnable(GL_BLEND);

    glTranslatef(0, 0, -originZ);

    double viewingRadius = cameraPose.distance / 4000.0 * 3000.0;
    if (viewingRadius < 100.0)
    {
        viewingRadius = 100.0;
    }

    double minResolution = 0.25;
    double centerResolution = cameraPose.distance / 100.0;
    double maxResolution = 1048576.0;

    if (imageryComboBox->currentText().compare("Map (Google)") == 0)
    {
        minResolution = 0.25;
    }
    else if (imageryComboBox->currentText().compare("Satellite (Google)") == 0)
    {
        minResolution = 0.5;
    }
    else if (imageryComboBox->currentText().compare("Satellite (Swisstopo)") == 0)
    {
        minResolution = 0.25;
        maxResolution = 0.25;
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

    imagery->draw3D(viewingRadius, resolution, originX, originY,
                    cameraPose.xOffset, cameraPose.yOffset, zone);

    if (prefetch)
    {
        if (resolution / 2.0 >= minResolution)
        {
            imagery->prefetch3D(viewingRadius / 2.0, resolution / 2.0,
                                originX, originY,
                                cameraPose.xOffset, cameraPose.yOffset, zone);
        }
        if (resolution * 2.0 <= maxResolution)
        {
            imagery->prefetch3D(viewingRadius * 2.0, resolution * 2.0,
                                originX, originY,
                                cameraPose.xOffset, cameraPose.yOffset, zone);
        }
    }

    glDisable(GL_BLEND);
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
QMap3DWidget::drawTarget(float x, float y, float z) const
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

    wireSphere(radius, 10, 10);

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

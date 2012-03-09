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
 *   @brief Head Up Display (HUD)
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QShowEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDesktopServices>
#include <QFileDialog>

#include <QDebug>
#include <cmath>
#include <qmath.h>
#include <limits>

#include "UASManager.h"
#include "UAS.h"
#include "HUD.h"
#include "MG.h"
#include "QGC.h"

// Fix for some platforms, e.g. windows
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

/**
 * @warning The HUD widget will not start painting its content automatically
 *          to update the view, start the auto-update by calling HUD::start().
 *
 * @param width
 * @param height
 * @param parent
 */
HUD::HUD(int width, int height, QWidget* parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
      uas(NULL),
      yawInt(0.0f),
      mode(tr("UNKNOWN MODE")),
      state(tr("UNKNOWN STATE")),
      fuelStatus(tr("00.0V (00m:00s)")),
      xCenterOffset(0.0f),
      yCenterOffset(0.0f),
      vwidth(200.0f),
      vheight(150.0f),
      vGaugeSpacing(50.0f),
      vPitchPerDeg(6.0f), ///< 4 mm y translation per degree)
      rawBuffer1(NULL),
      rawBuffer2(NULL),
      rawImage(NULL),
      rawLastIndex(0),
      rawExpectedBytes(0),
      bytesPerLine(1),
      imageStarted(false),
      receivedDepth(8),
      receivedChannels(1),
      receivedWidth(640),
      receivedHeight(480),
      defaultColor(QColor(70, 200, 70)),
      setPointColor(QColor(200, 20, 200)),
      warningColor(Qt::yellow),
      criticalColor(Qt::red),
      infoColor(QColor(20, 200, 20)),
      fuelColor(criticalColor),
      warningBlinkRate(5),
      refreshTimer(new QTimer(this)),
      noCamera(true),
      hardwareAcceleration(true),
      strongStrokeWidth(1.5f),
      normalStrokeWidth(1.0f),
      fineStrokeWidth(0.5f),
      waypointName(""),
      roll(0.0f),
      pitch(0.0f),
      yaw(0.0f),
      rollLP(0.0f),
      pitchLP(0.0f),
      yawLP(0.0f),
      yawDiff(0.0f),
      xPos(0.0),
      yPos(0.0),
      zPos(0.0),
      xSpeed(0.0),
      ySpeed(0.0),
      zSpeed(0.0),
      lastSpeedUpdate(0),
      totalSpeed(0.0),
      totalAcc(0.0),
      lat(0.0),
      lon(0.0),
      alt(0.0),
      load(0.0f),
      offlineDirectory(""),
      nextOfflineImage(""),
      hudInstrumentsEnabled(true),
      videoEnabled(false),
      xImageFactor(1.0),
      yImageFactor(1.0),
      imageRequested(false)
{
    // Set auto fill to false
    setAutoFillBackground(false);

    // Set minimum size
    setMinimumSize(80, 60);
    // Set preferred size
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scalingFactor = this->width()/vwidth;

    // Fill with black background
    QImage fill = QImage(width, height, QImage::Format_Indexed8);
    fill.setNumColors(3);
    fill.setColor(0, qRgb(0, 0, 0));
    fill.setColor(1, qRgb(0, 0, 0));
    fill.setColor(2, qRgb(0, 0, 0));
    fill.fill(0);

    //QString imagePath = "/Users/user/Desktop/frame0000.png";
    //qDebug() << __FILE__ << __LINE__ << "template image:" << imagePath;
    //fill = QImage(imagePath);

    glImage = QGLWidget::convertToGLFormat(fill);

    // Refresh timer
    refreshTimer->setInterval(updateInterval);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(paintHUD()));

    // Resize to correct size and fill with image
    resize(this->width(), this->height());
    //glDrawPixels(glImage.width(), glImage.height(), GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // Set size once
    //setFixedSize(fill.size());
    //setMinimumSize(fill.size());
    //setMaximumSize(fill.size());
    // Lock down the size
    //setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    fontDatabase = QFontDatabase();
    const QString fontFileName = ":/general/vera.ttf"; ///< Font file is part of the QRC file and compiled into the app
    const QString fontFamilyName = "Bitstream Vera Sans";
    if(!QFile::exists(fontFileName)) qDebug() << "ERROR! font file: " << fontFileName << " DOES NOT EXIST!";

    fontDatabase.addApplicationFont(fontFileName);
    font = fontDatabase.font(fontFamilyName, "Roman", qMax(5,(int)(10.0f*scalingFactor*1.2f+0.5f)));
    QFont* fontPtr = &font;
    if (!fontPtr) {
        qDebug() << "ERROR! FONT NOT LOADED!";
    } else {
        if (font.family() != fontFamilyName) qDebug() << "ERROR! WRONG FONT LOADED: " << fontFamilyName;
    }

    // Connect with UAS
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    createActions();

    if (UASManager::instance()->getActiveUAS() != NULL) setActiveUAS(UASManager::instance()->getActiveUAS());
}

HUD::~HUD()
{
    refreshTimer->stop();
}

QSize HUD::sizeHint() const
{
    return QSize(width(), (width()*3.0f)/4);
}

void HUD::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    QGLWidget::showEvent(event);
    refreshTimer->start(updateInterval);
    emit visibilityChanged(true);
}

void HUD::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    refreshTimer->stop();
    QGLWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void HUD::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    // Update actions
    enableHUDAction->setChecked(hudInstrumentsEnabled);
    enableVideoAction->setChecked(videoEnabled);

    menu.addAction(enableHUDAction);
    //menu.addAction(selectHUDColorAction);
    menu.addAction(enableVideoAction);
    menu.addAction(selectOfflineDirectoryAction);
    //menu.addAction(selectVideoChannelAction);
    menu.exec(event->globalPos());
}

void HUD::createActions()
{
    enableHUDAction = new QAction(tr("Enable HUD"), this);
    enableHUDAction->setStatusTip(tr("Show the HUD instruments in this window"));
    enableHUDAction->setCheckable(true);
    enableHUDAction->setChecked(hudInstrumentsEnabled);
    connect(enableHUDAction, SIGNAL(triggered(bool)), this, SLOT(enableHUDInstruments(bool)));

    enableVideoAction = new QAction(tr("Enable Video Live feed"), this);
    enableVideoAction->setStatusTip(tr("Show the video live feed"));
    enableVideoAction->setCheckable(true);
    enableVideoAction->setChecked(videoEnabled);
    connect(enableVideoAction, SIGNAL(triggered(bool)), this, SLOT(enableVideo(bool)));

    selectOfflineDirectoryAction = new QAction(tr("Select image log"), this);
    selectOfflineDirectoryAction->setStatusTip(tr("Load previously logged images into simulation / replay"));
    connect(selectOfflineDirectoryAction, SIGNAL(triggered()), this, SLOT(selectOfflineDirectory()));
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HUD::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL) {
        // Disconnect any previously connected active MAV
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
        disconnect(this->uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString)));
        disconnect(this->uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(this->uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));

        disconnect(this->uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int, int)));

        // Try to disconnect the image link
        UAS* u = dynamic_cast<UAS*>(this->uas);
        if (u) {
            disconnect(u, SIGNAL(imageStarted(quint64)), this, SLOT(startImage(quint64)));
            disconnect(u, SIGNAL(imageReady(UASInterface*)), this, SLOT(copyImage()));
        }
    }

    if (uas) {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
        connect(uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString)));
        connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        connect(uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));

        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int, int)));

        // Try to connect the image link
        UAS* u = dynamic_cast<UAS*>(uas);
        if (u) {
            connect(u, SIGNAL(imageStarted(quint64)), this, SLOT(startImage(quint64)));
            connect(u, SIGNAL(imageReady(UASInterface*)), this, SLOT(copyImage()));
        }

        // Set new UAS
        this->uas = uas;
    }
}

//void HUD::updateAttitudeThrustSetPoint(UASInterface* uas, double rollDesired, double pitchDesired, double yawDesired, double thrustDesired, quint64 msec)
//{
////    updateValue(uas, "roll desired", rollDesired, msec);
////    updateValue(uas, "pitch desired", pitchDesired, msec);
////    updateValue(uas, "yaw desired", yawDesired, msec);
////    updateValue(uas, "thrust desired", thrustDesired, msec);
//}

void HUD::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    this->roll = roll;
    this->pitch = pitch;
    this->yaw = yaw;
}

void HUD::updateAttitude(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    attitudes.insert(component, QVector3D(roll, pitch, yaw));
}

void HUD::updateBattery(UASInterface* uas, double voltage, double percent, int seconds)
{
    Q_UNUSED(uas);
    fuelStatus = tr("BAT [%1% | %2V] (%3:%4)").arg(percent, 2, 'f', 0, QChar('0')).arg(voltage, 4, 'f', 1, QChar('0')).arg(seconds/60, 2, 10, QChar('0')).arg(seconds%60, 2, 10, QChar('0'));
    if (percent < 20.0f) {
        fuelColor = warningColor;
    } else if (percent < 10.0f) {
        fuelColor = criticalColor;
    } else {
        fuelColor = infoColor;
    }
}

void HUD::receiveHeartbeat(UASInterface*)
{
}

void HUD::updateThrust(UASInterface* uas, double thrust)
{
    Q_UNUSED(uas);
    Q_UNUSED(thrust);
//    updateValue(uas, "thrust", thrust, MG::TIME::getGroundTimeNow());
}

void HUD::updateLocalPosition(UASInterface* uas,double x,double y,double z,quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    this->xPos = x;
    this->yPos = y;
    this->zPos = z;
}

void HUD::updateGlobalPosition(UASInterface* uas,double lat, double lon, double altitude, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    this->lat = lat;
    this->lon = lon;
    this->alt = altitude;
}

void HUD::updateSpeed(UASInterface* uas,double x,double y,double z,quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    this->xSpeed = x;
    this->ySpeed = y;
    this->zSpeed = z;
    double newTotalSpeed = sqrt(xSpeed*xSpeed + ySpeed*ySpeed + zSpeed*zSpeed);
    totalAcc = (newTotalSpeed - totalSpeed) / ((double)(lastSpeedUpdate - timestamp)/1000.0);
    totalSpeed = newTotalSpeed;
}

/**
 * Updates the current system state, but only if the uas matches the currently monitored uas.
 *
 * @param uas the system the state message originates from
 * @param state short state text, displayed in HUD
 */
void HUD::updateState(UASInterface* uas,QString state)
{
    // Only one UAS is connected at a time
    Q_UNUSED(uas);
    this->state = state;
}

/**
 * Updates the current system mode, but only if the uas matches the currently monitored uas.
 *
 * @param uas the system the state message originates from
 * @param mode short mode text, displayed in HUD
 */
void HUD::updateMode(int id,QString mode, QString description)
{
    // Only one UAS is connected at a time
    Q_UNUSED(id);
    Q_UNUSED(description);
    this->mode = mode;
}

void HUD::updateLoad(UASInterface* uas, double load)
{
    Q_UNUSED(uas);
    this->load = load;
    //updateValue(uas, "load", load, MG::TIME::getGroundTimeNow());
}

/**
 * @param y coordinate in pixels to be converted to reference mm units
 * @return the screen coordinate relative to the QGLWindow origin
 */
float HUD::refToScreenX(float x)
{
    //qDebug() << "sX: " << (scalingFactor * x);
    return (scalingFactor * x);
}
/**
 * @param x coordinate in pixels to be converted to reference mm units
 * @return the screen coordinate relative to the QGLWindow origin
 */
float HUD::refToScreenY(float y)
{
    //qDebug() << "sY: " << (scalingFactor * y);
    return (scalingFactor * y);
}

/**
 * This functions works in the OpenGL view, which is already translated by
 * the x and y center offsets.
 *
 */
void HUD::paintCenterBackground(float roll, float pitch, float yaw)
{
    // Center indicator is 100 mm wide
    float referenceWidth = 70.0;
    float referenceHeight = 70.0;

    // HUD is assumed to be 200 x 150 mm
    // so that positions can be hardcoded
    // but can of course be scaled.

    double referencePositionX = vwidth / 2.0 - referenceWidth/2.0;
    double referencePositionY = vheight / 2.0 - referenceHeight/2.0;

    //this->width()/2.0+(xCenterOffset*scalingFactor), this->height()/2.0+(yCenterOffset*scalingFactor);

    setupGLView(referencePositionX, referencePositionY, referenceWidth, referenceHeight);

    // Store current position in the model view
    // the position will be restored after drawing
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Move to the center of the window
    glTranslatef(referenceWidth/2.0f,referenceHeight/2.0f,0);

    // Move based on the yaw difference
    glTranslatef(yaw, 0.0f, 0.0f);

    // Rotate based on the bank
    glRotatef((roll/M_PI)*180.0f, 0.0f, 0.0f, 1.0f);

    // Translate in the direction of the rotation based
    // on the pitch. On the 777, a pitch of 1 degree = 2 mm
    //glTranslatef(0, ((-pitch/M_PI)*180.0f * vPitchPerDeg), 0);
    glTranslatef(0.0f, (-pitch * vPitchPerDeg * 16.5f), 0.0f);

    // Ground
    glColor3ub(179,102,0);

    glBegin(GL_POLYGON);
    glVertex2f(-300,-300);
    glVertex2f(-300,0);
    glVertex2f(300,0);
    glVertex2f(300,-300);
    glVertex2f(-300,-300);
    glEnd();

    // Sky
    glColor3ub(0,153,204);

    glBegin(GL_POLYGON);
    glVertex2f(-300,0);
    glVertex2f(-300,300);
    glVertex2f(300,300);
    glVertex2f(300,0);
    glVertex2f(-300,0);

    glEnd();
}

/**
 * Paint text on top of the image and OpenGL drawings
 *
 * @param text chars to write
 * @param color text color
 * @param fontSize text size in mm
 * @param refX position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
 * @param refY position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
 */
void HUD::paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter)
{
    QPen prevPen = painter->pen();
    float pPositionX = refToScreenX(refX) - (fontSize*scalingFactor*0.072f);
    float pPositionY = refToScreenY(refY) - (fontSize*scalingFactor*0.212f);

    QFont font("Bitstream Vera Sans");
    // Enforce minimum font size of 5 pixels
    int fSize = qMax(5, (int)(fontSize*scalingFactor*1.26f));
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

void HUD::initializeGL()
{
    bool antialiasing = true;

    // Antialiasing setup
    if(antialiasing) {
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);

        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_BLEND);
        glDisable(GL_POINT_SMOOTH);
        glDisable(GL_LINE_SMOOTH);
    }
}

/**
 * @param referencePositionX horizontal position in the reference mm-unit space
 * @param referencePositionY horizontal position in the reference mm-unit space
 * @param referenceWidth width in the reference mm-unit space
 * @param referenceHeight width in the reference mm-unit space
 */
void HUD::setupGLView(float referencePositionX, float referencePositionY, float referenceWidth, float referenceHeight)
{
    int pixelWidth  = (int)(referenceWidth * scalingFactor);
    int pixelHeight = (int)(referenceHeight * scalingFactor);
    // Translate and scale the GL view in the virtual reference coordinate units on the screen
    int pixelPositionX = (int)((referencePositionX * scalingFactor) + xCenterOffset);
    int pixelPositionY = this->height() - (referencePositionY * scalingFactor) + yCenterOffset - pixelHeight;

    //qDebug() << "Pixel x" << pixelPositionX << "pixelY" << pixelPositionY;
    //qDebug() << "xCenterOffset:" << xCenterOffset << "yCenterOffest" << yCenterOffset


    //The viewport is established at the correct pixel position and clips everything
    // out of the desired instrument location
    glViewport(pixelPositionX, pixelPositionY, pixelWidth, pixelHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // The ortho projection is setup in a way that so that the drawing is done in the
    // reference coordinate space
    glOrtho(0, referenceWidth, 0, referenceHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glScalef(scaleX, scaleY, 1.0f);
}

void HUD::paintRollPitchStrips()
{
}


void HUD::paintEvent(QPaintEvent *event)
{
    // Event is not needed
    // the event is ignored as this widget
    // is refreshed automatically
    Q_UNUSED(event);
}

void HUD::paintHUD()
{
    if (isVisible()) {
        //    static quint64 interval = 0;
        //    qDebug() << "INTERVAL:" << MG::TIME::getGroundTimeNow() - interval << __FILE__ << __LINE__;
        //    interval = MG::TIME::getGroundTimeNow();

#if (QGC_EVENTLOOP_DEBUG)
        qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif

        // Read out most important values to limit hash table lookups
        // Low-pass roll, pitch and yaw
        rollLP = rollLP * 0.2f + 0.8f * roll;
        pitchLP = pitchLP * 0.2f + 0.8f * pitch;
        yawLP = yawLP * 0.2f + 0.8f * yaw;

        // Translate for yaw
        const float maxYawTrans = 60.0f;

        float newYawDiff = yawDiff;
        if (isinf(newYawDiff)) newYawDiff = yawDiff;
        if (newYawDiff > M_PI) newYawDiff = newYawDiff - M_PI;

        if (newYawDiff < -M_PI) newYawDiff = newYawDiff + M_PI;

        newYawDiff = yawDiff * 0.8 + newYawDiff * 0.2;

        yawDiff = newYawDiff;

        yawInt += newYawDiff;

        if (yawInt > M_PI) yawInt = (float)M_PI;
        if (yawInt < -M_PI) yawInt = (float)-M_PI;

        float yawTrans = yawInt * (float)maxYawTrans;
        yawInt *= 0.6f;

        if ((yawTrans < 5.0) && (yawTrans > -5.0)) yawTrans = 0;

        // Negate to correct direction
        yawTrans = -yawTrans;

        //qDebug() << "yaw translation" << yawTrans << "integral" << yawInt << "difference" << yawDiff << "yaw" << yaw;

        // Update scaling factor
        // adjust scaling to fit both horizontally and vertically
        scalingFactor = this->width()/vwidth;
        double scalingFactorH = this->height()/vheight;
        if (scalingFactorH < scalingFactor) scalingFactor = scalingFactorH;



        // OPEN GL PAINTING
        // Store model view matrix to be able to reset it to the previous state
        makeCurrent();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Fill with black background
        if (videoEnabled) {
            if (nextOfflineImage != "" && QFileInfo(nextOfflineImage).exists()) {
                qDebug() << __FILE__ << __LINE__ << "template image:" << nextOfflineImage;
                QImage fill = QImage(nextOfflineImage);

                glImage = QGLWidget::convertToGLFormat(fill);

                // Reset to save load efforts
                nextOfflineImage = "";
            }

        }

        if (dataStreamEnabled || videoEnabled)
        {
            glRasterPos2i(0, 0);

            xImageFactor = width() / (float)glImage.width();
            yImageFactor = height() / (float)glImage.height();
            float imageFactor = qMin(xImageFactor, yImageFactor);
            glPixelZoom(imageFactor, imageFactor);
            // Resize to correct size and fill with image
            glDrawPixels(glImage.width(), glImage.height(), GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());
            //qDebug() << "DRAWING GL IMAGE";
        } else {
            // Blue / brown background
            paintCenterBackground(roll, pitch, yawTrans);
        }

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        // END OF OPENGL PAINTING

        if (hudInstrumentsEnabled) {

            //glEnable(GL_MULTISAMPLE);

            // QT PAINTING
            //makeCurrent();
            QPainter painter;
            painter.begin(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
            painter.translate((this->vwidth/2.0+xCenterOffset)*scalingFactor, (this->vheight/2.0+yCenterOffset)*scalingFactor);



            // COORDINATE FRAME IS NOW (0,0) at CENTER OF WIDGET


            // Draw all fixed indicators
            // MODE
            paintText(mode, infoColor, 2.0f, (-vwidth/2.0) + 10, -vheight/2.0 + 10, &painter);
            // STATE
            paintText(state, infoColor, 2.0f, (-vwidth/2.0) + 10, -vheight/2.0 + 15, &painter);
            // BATTERY
            paintText(fuelStatus, fuelColor, 2.0f, (-vwidth/2.0) + 10, -vheight/2.0 + 20, &painter);
            // Waypoint
            paintText(waypointName, defaultColor, 2.0f, (-vwidth/3.0) + 10, +vheight/3.0 + 15, &painter);

            // YAW INDICATOR
            //
            //      .
            //    .   .
            //   .......
            //
            const float yawIndicatorWidth = 4.0f;
            const float yawIndicatorY = vheight/2.0f - 10.0f;
            QPolygon yawIndicator(4);
            yawIndicator.setPoint(0, QPoint(refToScreenX(0.0f), refToScreenY(yawIndicatorY)));
            yawIndicator.setPoint(1, QPoint(refToScreenX(yawIndicatorWidth/2.0f), refToScreenY(yawIndicatorY+yawIndicatorWidth)));
            yawIndicator.setPoint(2, QPoint(refToScreenX(-yawIndicatorWidth/2.0f), refToScreenY(yawIndicatorY+yawIndicatorWidth)));
            yawIndicator.setPoint(3, QPoint(refToScreenX(0.0f), refToScreenY(yawIndicatorY)));
            painter.setPen(defaultColor);
            painter.drawPolyline(yawIndicator);

            // CENTER

            // HEADING INDICATOR
            //
            //    __      __
            //       \/\/
            //
            const float hIndicatorWidth = 7.0f;
            const float hIndicatorY = -25.0f;
            const float hIndicatorYLow = hIndicatorY + hIndicatorWidth / 6.0f;
            const float hIndicatorSegmentWidth = hIndicatorWidth / 7.0f;
            QPolygon hIndicator(7);
            hIndicator.setPoint(0, QPoint(refToScreenX(0.0f-hIndicatorWidth/2.0f), refToScreenY(hIndicatorY)));
            hIndicator.setPoint(1, QPoint(refToScreenX(0.0f-hIndicatorWidth/2.0f+hIndicatorSegmentWidth*1.75f), refToScreenY(hIndicatorY)));
            hIndicator.setPoint(2, QPoint(refToScreenX(0.0f-hIndicatorSegmentWidth*1.0f), refToScreenY(hIndicatorYLow)));
            hIndicator.setPoint(3, QPoint(refToScreenX(0.0f), refToScreenY(hIndicatorY)));
            hIndicator.setPoint(4, QPoint(refToScreenX(0.0f+hIndicatorSegmentWidth*1.0f), refToScreenY(hIndicatorYLow)));
            hIndicator.setPoint(5, QPoint(refToScreenX(0.0f+hIndicatorWidth/2.0f-hIndicatorSegmentWidth*1.75f), refToScreenY(hIndicatorY)));
            hIndicator.setPoint(6, QPoint(refToScreenX(0.0f+hIndicatorWidth/2.0f), refToScreenY(hIndicatorY)));
            painter.setPen(defaultColor);
            painter.drawPolyline(hIndicator);


            // SETPOINT
            const float centerWidth = 4.0f;
            painter.setPen(defaultColor);
            painter.setBrush(Qt::NoBrush);
            // TODO
            //painter.drawEllipse(QPointF(refToScreenX(qMin(10.0f, values.value("roll desired", 0.0f) * 10.0f)), refToScreenY(qMin(10.0f, values.value("pitch desired", 0.0f) * 10.0f))), refToScreenX(centerWidth/2.0f), refToScreenX(centerWidth/2.0f));

            const float centerCrossWidth = 10.0f;
            // left
            painter.drawLine(QPointF(refToScreenX(-centerWidth / 2.0f), refToScreenY(0.0f)), QPointF(refToScreenX(-centerCrossWidth / 2.0f), refToScreenY(0.0f)));
            // right
            painter.drawLine(QPointF(refToScreenX(centerWidth / 2.0f), refToScreenY(0.0f)), QPointF(refToScreenX(centerCrossWidth / 2.0f), refToScreenY(0.0f)));
            // top
            painter.drawLine(QPointF(refToScreenX(0.0f), refToScreenY(-centerWidth / 2.0f)), QPointF(refToScreenX(0.0f), refToScreenY(-centerCrossWidth / 2.0f)));



            // COMPASS
            const float compassY = -vheight/2.0f + 10.0f;
            QRectF compassRect(QPointF(refToScreenX(-5.0f), refToScreenY(compassY)), QSizeF(refToScreenX(10.0f), refToScreenY(5.0f)));
            painter.setBrush(Qt::NoBrush);
            painter.setPen(Qt::SolidLine);
            painter.setPen(defaultColor);
            painter.drawRoundedRect(compassRect, 2, 2);
            QString yawAngle;

            //    const float yawDeg = ((values.value("yaw", 0.0f)/M_PI)*180.0f)+180.f;

            // YAW is in compass-human readable format, so 0 - 360deg. This is normal in aviation, not -180 - +180.
            const float yawDeg = ((yawLP/M_PI)*180.0f)+180.0f+180.0f;
            int yawCompass = static_cast<int>(yawDeg) % 360;
            yawAngle.sprintf("%03d", yawCompass);
            paintText(yawAngle, defaultColor, 3.5f, -4.3f, compassY+ 0.97f, &painter);

            // CHANGE RATE STRIPS
            drawChangeRateStrip(-51.0f, -50.0f, 15.0f, -1.0f, 1.0f, -zSpeed, &painter);

            // CHANGE RATE STRIPS
            drawChangeRateStrip(49.0f, -50.0f, 15.0f, -1.0f, 1.0f, totalAcc, &painter);

            // GAUGES

            // Left altitude gauge
            float gaugeAltitude;

            if (this->alt != 0) {
                gaugeAltitude = alt;
            } else {
                gaugeAltitude = -zPos;
            }

            drawChangeIndicatorGauge(-vGaugeSpacing, -15.0f, 10.0f, 2.0f, gaugeAltitude, defaultColor, &painter, false);

            // Right speed gauge
            drawChangeIndicatorGauge(vGaugeSpacing, -15.0f, 10.0f, 5.0f, totalSpeed, defaultColor, &painter, false);


            // Waypoint name
            if (waypointName != "") paintText(waypointName, defaultColor, 2.0f, (-vwidth/3.0) + 10, +vheight/3.0 + 15, &painter);

            // MOVING PARTS


            painter.translate(refToScreenX(yawTrans), 0);

            // Old single-component pitch drawing
//            // Rotate view and draw all roll-dependent indicators
//            painter.rotate((rollLP/M_PI)* -180.0f);

//            painter.translate(0, (-pitchLP/(float)M_PI)* -180.0f * refToScreenY(1.8f));

//            //qDebug() << "ROLL" << roll << "PITCH" << pitch << "YAW DIFF" << valuesDot.value("roll", 0.0f);

//            // PITCH

//            paintPitchLines(pitchLP, &painter);

            QColor attColor = painter.pen().color();

            // Draw multi-component attitude
            foreach (QVector3D att, attitudes.values())
            {
                attColor = attColor.darker(200);
                painter.setPen(attColor);
                // Rotate view and draw all roll-dependent indicators
                painter.rotate((att.x()/M_PI)* -180.0f);

                painter.translate(0, (-att.y()/(float)M_PI)* -180.0f * refToScreenY(1.8f));

                //qDebug() << "ROLL" << roll << "PITCH" << pitch << "YAW DIFF" << valuesDot.value("roll", 0.0f);

                // PITCH

                paintPitchLines(att.y(), &painter);
                painter.translate(0, -(-att.y()/(float)M_PI)* -180.0f * refToScreenY(1.8f));
                painter.rotate(-(att.x()/M_PI)* -180.0f);
            }

            painter.end();
        } else {
            QPainter painter;
            painter.begin(this);
            painter.end();
        }
        //glDisable(GL_MULTISAMPLE);



        //glFlush();
    }
}


/**
 * @param pitch pitch angle in degrees (-180 to 180)
 */
void HUD::paintPitchLines(float pitch, QPainter* painter)
{
    QString label;

    const float yDeg = vPitchPerDeg;
    const float lineDistance = 5.0f; ///< One pitch line every 10 degrees
    const float posIncrement = yDeg * lineDistance;
    float posY = posIncrement;
    const float posLimit = sqrt(pow(vwidth, 2.0f) + pow(vheight, 2.0f));

    const float offsetAbs = pitch * yDeg;

    float offset = pitch;
    if (offset < 0) offset = -offset;
    int offsetCount = 0;
    while (offset > lineDistance) {
        offset -= lineDistance;
        offsetCount++;
    }

    int iPos = (int)(0.5f + lineDistance); ///< The first line
    int iNeg = (int)(-0.5f - lineDistance); ///< The first line

    offset *= yDeg;


    painter->setPen(defaultColor);

    posY = -offsetAbs + posIncrement; //+ 100;// + lineDistance;

    while (posY < posLimit) {
        paintPitchLinePos(label.sprintf("%3d", iPos), 0.0f, -posY, painter);
        posY += posIncrement;
        iPos += (int)lineDistance;
    }



    // HORIZON
    //
    //    ------------    ------------
    //
    const float pitchWidth = 30.0f;
    const float pitchGap = pitchWidth / 2.5f;
    const QColor horizonColor = defaultColor;
    const float diagonal = sqrt(pow(vwidth, 2.0f) + pow(vheight, 2.0f));
    const float lineWidth = refLineWidthToPen(0.5f);

    // Left horizon
    drawLine(0.0f-diagonal, offsetAbs, 0.0f-pitchGap/2.0f, offsetAbs, lineWidth, horizonColor, painter);
    // Right horizon
    drawLine(0.0f+pitchGap/2.0f, offsetAbs, 0.0f+diagonal, offsetAbs, lineWidth, horizonColor, painter);



    label.clear();

    posY = offsetAbs  + posIncrement;


    while (posY < posLimit) {
        paintPitchLineNeg(label.sprintf("%3d", iNeg), 0.0f, posY, painter);
        posY += posIncrement;
        iNeg -= (int)lineDistance;
    }
}

void HUD::paintPitchLinePos(QString text, float refPosX, float refPosY, QPainter* painter)
{
    //painter->setPen(QPen(QBrush, normalStrokeWidth));

    const float pitchWidth = 30.0f;
    const float pitchGap = pitchWidth / 2.5f;
    const float pitchHeight = pitchWidth / 12.0f;
    const float textSize = pitchHeight * 1.1f;
    const float lineWidth = 0.5f;

    // Positive pitch indicator:
    //
    //      _______      _______
    //     |10                  |
    //

    // Left vertical line
    drawLine(refPosX-pitchWidth/2.0f, refPosY, refPosX-pitchWidth/2.0f, refPosY+pitchHeight, lineWidth, defaultColor, painter);
    // Left horizontal line
    drawLine(refPosX-pitchWidth/2.0f, refPosY, refPosX-pitchGap/2.0f, refPosY, lineWidth, defaultColor, painter);
    // Text left
    paintText(text, defaultColor, textSize, refPosX-pitchWidth/2.0 + 0.75f, refPosY + pitchHeight - 1.75f, painter);

    // Right vertical line
    drawLine(refPosX+pitchWidth/2.0f, refPosY, refPosX+pitchWidth/2.0f, refPosY+pitchHeight, lineWidth, defaultColor, painter);
    // Right horizontal line
    drawLine(refPosX+pitchWidth/2.0f, refPosY, refPosX+pitchGap/2.0f, refPosY, lineWidth, defaultColor, painter);
}

void HUD::paintPitchLineNeg(QString text, float refPosX, float refPosY, QPainter* painter)
{
    const float pitchWidth = 30.0f;
    const float pitchGap = pitchWidth / 2.5f;
    const float pitchHeight = pitchWidth / 12.0f;
    const float textSize = pitchHeight * 1.1f;
    const float segmentWidth = ((pitchWidth - pitchGap)/2.0f) / 7.0f; ///< Four lines and three gaps -> 7 segments

    const float lineWidth = 0.1f;

    // Negative pitch indicator:
    //
    //      -10
    //     _ _ _ _|     |_ _ _ _
    //
    //

    // Left vertical line
    drawLine(refPosX-pitchGap/2.0, refPosY, refPosX-pitchGap/2.0, refPosY-pitchHeight, lineWidth, defaultColor, painter);
    // Left horizontal line with four segments
    for (int i = 0; i < 7; i+=2) {
        drawLine(refPosX-pitchWidth/2.0+(i*segmentWidth), refPosY, refPosX-pitchWidth/2.0+(i*segmentWidth)+segmentWidth, refPosY, lineWidth, defaultColor, painter);
    }
    // Text left
    paintText(text, defaultColor, textSize, refPosX-pitchWidth/2.0f + 0.75f, refPosY + pitchHeight - 1.75f, painter);

    // Right vertical line
    drawLine(refPosX+pitchGap/2.0, refPosY, refPosX+pitchGap/2.0, refPosY-pitchHeight, lineWidth, defaultColor, painter);
    // Right horizontal line with four segments
    for (int i = 0; i < 7; i+=2) {
        drawLine(refPosX+pitchWidth/2.0f-(i*segmentWidth), refPosY, refPosX+pitchWidth/2.0f-(i*segmentWidth)-segmentWidth, refPosY, lineWidth, defaultColor, painter);
    }
}

void rotatePointClockWise(QPointF& p, float angle)
{
    // Standard 2x2 rotation matrix, counter-clockwise
    //
    //   |  cos(phi)   sin(phi) |
    //   | -sin(phi)   cos(phi) |
    //

    //p.setX(cos(angle) * p.x() + sin(angle) * p.y());
    //p.setY(-sin(angle) * p.x() + cos(angle) * p.y());


    p.setX(cos(angle) * p.x() + sin(angle)* p.y());
    p.setY((-1.0f * sin(angle) * p.x()) + cos(angle) * p.y());
}

float HUD::refLineWidthToPen(float line)
{
    return line * 2.50f;
}

/**
 * Rotate a polygon around a point
 *
 * @param p polygon to rotate
 * @param origin the rotation center
 * @param angle rotation angle, in radians
 * @return p Polygon p rotated by angle around the origin point
 */
void HUD::rotatePolygonClockWiseRad(QPolygonF& p, float angle, QPointF origin)
{
    // Standard 2x2 rotation matrix, counter-clockwise
    //
    //   |  cos(phi)   sin(phi) |
    //   | -sin(phi)   cos(phi) |
    //
    for (int i = 0; i < p.size(); i++) {
        QPointF curr = p.at(i);

        const float x = curr.x();
        const float y = curr.y();

        curr.setX(((cos(angle) * (x-origin.x())) + (-sin(angle) * (y-origin.y()))) + origin.x());
        curr.setY(((sin(angle) * (x-origin.x())) + (cos(angle) * (y-origin.y()))) + origin.y());
        p.replace(i, curr);
    }
}

void HUD::drawPolygon(QPolygonF refPolygon, QPainter* painter)
{
    // Scale coordinates
    QPolygonF draw(refPolygon.size());
    for (int i = 0; i < refPolygon.size(); i++) {
        QPointF curr;
        curr.setX(refToScreenX(refPolygon.at(i).x()));
        curr.setY(refToScreenY(refPolygon.at(i).y()));
        draw.replace(i, curr);
    }
    painter->drawPolygon(draw);
}

void HUD::drawChangeRateStrip(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter)
{
    QBrush brush(defaultColor, Qt::NoBrush);
    painter->setBrush(brush);
    QPen rectPen(Qt::SolidLine);
    rectPen.setWidth(0);
    rectPen.setColor(defaultColor);
    painter->setPen(rectPen);

    float scaledValue = value;

    // Saturate value
    if (value > maxRate) scaledValue = maxRate;
    if (value < minRate) scaledValue = minRate;

    //           x (Origin: xRef, yRef)
    //           -
    //           |
    //           |
    //           |
    //           =
    //           |
    //   -0.005 >|
    //           |
    //           -

    const float width = height / 8.0f;
    const float lineWidth = 0.5f;

    // Indicator lines
    // Top horizontal line
    drawLine(xRef, yRef, xRef+width, yRef, lineWidth, defaultColor, painter);
    // Vertical main line
    drawLine(xRef+width/2.0f, yRef, xRef+width/2.0f, yRef+height, lineWidth, defaultColor, painter);
    // Zero mark
    drawLine(xRef, yRef+height/2.0f, xRef+width, yRef+height/2.0f, lineWidth, defaultColor, painter);
    // Horizontal bottom line
    drawLine(xRef, yRef+height, xRef+width, yRef+height, lineWidth, defaultColor, painter);

    // Text
    QString label;
    label.sprintf("< %+06.2f", value);
    paintText(label, defaultColor, 3.0f, xRef+width/2.0f, yRef+height-((scaledValue - minRate)/(maxRate-minRate))*height - 1.6f, painter);
}

//void HUD::drawSystemIndicator(float xRef, float yRef, int maxNum, float maxWidth, float maxHeight, QPainter* painter)
//{
//    Q_UNUSED(maxWidth);
//    Q_UNUSED(maxHeight);
//    if (values.size() > 0)
//    {
//        QString selectedKey = values.begin().key();
//        //   | | | | | |
//        //   | | | | | |
//        //   x speed: 2.54

//        // One column per value
//        QMapIterator<QString, float> value(values);

//        float x = xRef;
//        float y = yRef;

//        const float vspacing = 1.0f;
//        float width = 1.5f;
//        float height = 1.5f;
//        const float hspacing = 0.6f;

//        // TODO ensure that instrument stays smaller than maxWidth and maxHeight


//        int i = 0;
//        while (value.hasNext() && i < maxNum)
//        {
//            value.next();
//            QBrush brush(Qt::SolidPattern);


//            if (value.value() < 0.01f && value.value() > -0.01f)
//            {
//                brush.setColor(Qt::gray);
//            }
//            else if (value.value() > 0.01f)
//            {
//                brush.setColor(Qt::blue);
//            }
//            else
//            {
//                brush.setColor(Qt::yellow);
//            }

//            painter->setBrush(brush);
//            painter->setPen(Qt::NoPen);

//            // Draw current value colormap
//            painter->drawRect(refToScreenX(x), refToScreenY(y), refToScreenX(width), refToScreenY(height));

//            // Draw change rate colormap
//            painter->drawRect(refToScreenX(x), refToScreenY(y+height+hspacing), refToScreenX(width), refToScreenY(height));

//            // Draw mean value colormap
//            painter->drawRect(refToScreenX(x), refToScreenY(y+2.0f*(height+hspacing)), refToScreenX(width), refToScreenY(height));

//            // Add spacing
//            x += width+vspacing;

//            // Iterate
//            i++;
//        }

//        // Draw detail label
//        QString detail = "NO DATA AVAILABLE";

//        if (values.contains(selectedKey))
//        {
//            detail = values.find(selectedKey).key();
//            detail.append(": ");
//            detail.append(QString::number(values.find(selectedKey).value()));
//        }
//        paintText(detail, QColor(255, 255, 255), 3.0f, xRef, yRef+3.0f*(height+hspacing)+1.0f, painter);
//    }
//}

void HUD::drawChangeIndicatorGauge(float xRef, float yRef, float radius, float expectedMaxChange, float value, const QColor& color, QPainter* painter, bool solid)
{
    // Draw the circle
    QPen circlePen(Qt::SolidLine);
    if (!solid) circlePen.setStyle(Qt::DotLine);
    circlePen.setWidth(refLineWidthToPen(0.5f));
    circlePen.setColor(defaultColor);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(circlePen);
    drawCircle(xRef, yRef, radius, 200.0f, 170.0f, 1.0f, color, painter);

    QString label;
    label.sprintf("%05.1f", value);

    // Draw the value
    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);

    // Draw the needle
    // Scale the rotation so that the gauge does one revolution
    // per max. change
    const float rangeScale = (2.0f * M_PI) / expectedMaxChange;
    const float maxWidth = radius / 10.0f;
    const float minWidth = maxWidth * 0.3f;

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(4, QPointF(xRef,               yRef-radius * 0.46f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));

    rotatePolygonClockWiseRad(p, value*rangeScale, QPointF(xRef, yRef));

    QBrush indexBrush;
    indexBrush.setColor(defaultColor);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::SolidLine);
    painter->setPen(defaultColor);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}

void HUD::drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter)
{
    QPen pen(Qt::SolidLine);
    pen.setWidth(refLineWidthToPen(width));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(QPoint(refToScreenX(refX1), refToScreenY(refY1)), QPoint(refToScreenX(refX2), refToScreenY(refY2)));
}

void HUD::drawEllipse(float refX, float refY, float radiusX, float radiusY, float startDeg, float endDeg, float lineWidth, const QColor& color, QPainter* painter)
{
    Q_UNUSED(startDeg);
    Q_UNUSED(endDeg);
    QPen pen(painter->pen().style());
    pen.setWidth(refLineWidthToPen(lineWidth));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawEllipse(QPointF(refToScreenX(refX), refToScreenY(refY)), refToScreenX(radiusX), refToScreenY(radiusY));
}

void HUD::drawCircle(float refX, float refY, float radius, float startDeg, float endDeg, float lineWidth, const QColor& color, QPainter* painter)
{
    drawEllipse(refX, refY, radius, radius, startDeg, endDeg, lineWidth, color, painter);
}

void HUD::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPolygonMode(GL_FRONT, GL_FILL);
    //FIXME
    paintHUD();
}

void HUD::selectWaypoint(int uasId, int id)
{
    Q_UNUSED(uasId);
    waypointName = tr("WP") + QString::number(id);
}

void HUD::setImageSize(int width, int height, int depth, int channels)
{
    // Allocate raw image in correct size
    if (width != receivedWidth || height != receivedHeight || depth != receivedDepth || channels != receivedChannels || image == NULL) {
        // Set new size
        if (width > 0) receivedWidth  = width;
        if (height > 0) receivedHeight = height;
        if (depth > 1) receivedDepth = depth;
        if (channels > 1) receivedChannels = channels;

        rawExpectedBytes = (receivedWidth * receivedHeight * receivedDepth * receivedChannels) / 8;
        bytesPerLine = rawExpectedBytes / receivedHeight;
        // Delete old buffers if necessary
        rawImage = NULL;
        if (rawBuffer1 != NULL) delete rawBuffer1;
        if (rawBuffer2 != NULL) delete rawBuffer2;

        rawBuffer1 = (unsigned char*)malloc(rawExpectedBytes);
        rawBuffer2 = (unsigned char*)malloc(rawExpectedBytes);
        rawImage = rawBuffer1;
        // TODO check if old image should be deleted

        // Set image format
        // 8 BIT GREYSCALE IMAGE
        if (depth <= 8 && channels == 1) {
            image = new QImage(receivedWidth, receivedHeight, QImage::Format_Indexed8);
            // Create matching color table
            image->setNumColors(256);
            for (int i = 0; i < 256; i++) {
                image->setColor(i, qRgb(i, i, i));
                //qDebug() << __FILE__ << __LINE__ << std::hex << i;
            }

        }
        // 32 BIT COLOR IMAGE WITH ALPHA VALUES (#ARGB)
        else {
            image = new QImage(receivedWidth, receivedHeight, QImage::Format_ARGB32);
        }

        // Fill first channel of image with black pixels
        image->fill(0);
        glImage = QGLWidget::convertToGLFormat(*image);

        qDebug() << __FILE__ << __LINE__ << "Setting up image";

        // Set size once
        setFixedSize(receivedWidth, receivedHeight);
        setMinimumSize(receivedWidth, receivedHeight);
        setMaximumSize(receivedWidth, receivedHeight);
        // Lock down the size
        //setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        //resize(receivedWidth, receivedHeight);
    }

}

void HUD::startImage(int imgid, int width, int height, int depth, int channels)
{
    Q_UNUSED(imgid);
    //qDebug() << "HUD: starting image (" << width << "x" << height << ", " << depth << "bits) with " << channels << "channels";

    // Copy previous image to screen if it hasn't been finished properly
    finishImage();

    // Reset image size if necessary
    setImageSize(width, height, depth, channels);
    imageStarted = true;
}

void HUD::finishImage()
{
    if (imageStarted) {
        commitRawDataToGL();
        imageStarted = false;
    }
}

void HUD::commitRawDataToGL()
{
    qDebug() << __FILE__ << __LINE__ << "Copying raw data to GL buffer:" << rawImage << receivedWidth << receivedHeight << image->format();
    if (image != NULL) {
        QImage::Format format = image->format();
        QImage* newImage = new QImage(rawImage, receivedWidth, receivedHeight, format);
        if (format == QImage::Format_Indexed8) {
            // Create matching color table
            newImage->setNumColors(256);
            for (int i = 0; i < 256; i++) {
                newImage->setColor(i, qRgb(i, i, i));
                //qDebug() << __FILE__ << __LINE__ << std::hex << i;
            }
        }

        glImage = QGLWidget::convertToGLFormat(*newImage);
        delete image;
        image = newImage;
        // Switch buffers
        if (rawImage == rawBuffer1) {
            rawImage = rawBuffer2;
            //qDebug() << "Now buffer 2";
        } else {
            rawImage = rawBuffer1;
            //qDebug() << "Now buffer 1";
        }
    }
    update();
}

void HUD::saveImage(QString fileName)
{
    image->save(fileName);
}

void HUD::saveImage()
{
    //Bring up popup
    QString fileName = "output.png";
    saveImage(fileName);
}

void HUD::startImage(quint64 timestamp)
{
    if (videoEnabled && offlineDirectory != "") {
        // Load and diplay image file
        nextOfflineImage = QString(offlineDirectory + "/%1.bmp").arg(timestamp);
    }
}

void HUD::selectOfflineDirectory()
{
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Select image directory"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation));
    if (fileName != "") {
        offlineDirectory = fileName;
    }
}

void HUD::enableHUDInstruments(bool enabled)
{
    hudInstrumentsEnabled = enabled;
}

void HUD::enableVideo(bool enabled)
{
    videoEnabled = enabled;
}

void HUD::setPixels(int imgid, const unsigned char* imageData, int length, int startIndex)
{
    Q_UNUSED(imgid);
    //    qDebug() << "at" << __FILE__ << __LINE__ << ": Received startindex" << startIndex << "and length" << length << "(" << startIndex+length << "of" << rawExpectedBytes << "bytes)";

    if (imageStarted)
    {
        //if (rawLastIndex != startIndex) qDebug() << "PACKET LOSS!";

        if (startIndex+length > rawExpectedBytes)
        {
            qDebug() << "HUD: OVERFLOW! startIndex:" << startIndex << "length:" << length << "image raw size" << ((receivedWidth * receivedHeight * receivedChannels * receivedDepth) / 8) - 1;
        }
        else
        {
            memcpy(rawImage+startIndex, imageData, length);

            rawLastIndex = startIndex+length;

            // Check if we just reached the end of the image
            if (startIndex+length == rawExpectedBytes)
            {
                //qDebug() << "HUD: END OF IMAGE REACHED!";
                finishImage();
                rawLastIndex = 0;
            }
        }

        //        for (int i = 0; i < length; i++)
        //        {
        //            for (int j = 0; j < receivedChannels; j++)
        //            {
        //                unsigned int x = (startIndex+i) % receivedWidth;
        //                unsigned int y = (unsigned int)((startIndex+i) / receivedWidth);
        //                qDebug() << "Setting pixel" << x << "," << y << "to" << (unsigned int)*(rawImage+startIndex+i);
        //            }
        //        }
    }
}

void HUD::copyImage()
{
    if (isVisible() && hudInstrumentsEnabled)
    {
        //qDebug() << "HUD::copyImage()";
        UAS* u = dynamic_cast<UAS*>(this->uas);
        if (u)
        {
            this->glImage = QGLWidget::convertToGLFormat(u->getImage());
        }
    }
}

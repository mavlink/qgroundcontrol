/*=====================================================================
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.
    
======================================================================*/

/**
 * @file
 *   @brief Implementation of one airstrip
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <cmath>
#include <QDateTime>
#include <QDebug>

#include "QGC.h"
#include "MG.h"
#include "UASManager.h"
#include "UASView.h"
#include "UASWaypointManager.h"
#include "ui_UASView.h"

UASView::UASView(UASInterface* uas, QWidget *parent) :
        QWidget(parent),
        startTime(0),
        timeRemaining(0),
        chargeLevel(0),
        uas(uas),
        load(0),
        state("UNKNOWN"),
        stateDesc(tr("Unknown system state")),
        mode("MAV_MODE_UNKNOWN"),
        thrust(0),
        isActive(false),
        x(0),
        y(0),
        z(0),
        totalSpeed(0),
        lat(0),
        lon(0),
        alt(0),
        groundDistance(0),
        localFrame(false),
        m_ui(new Ui::UASView)
{
    m_ui->setupUi(this);
    
    // Setup communication
    //connect(uas, SIGNAL(valueChanged(int,QString,double,quint64)), this, SLOT(receiveValue(int,QString,double,quint64)));
    connect(uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
    connect(uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));
    connect(uas, SIGNAL(thrustChanged(UASInterface*, double)), this, SLOT(updateThrust(UASInterface*, double)));
    connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString,QString)));
    connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
    connect(uas, SIGNAL(loadChanged(UASInterface*, double)), this, SLOT(updateLoad(UASInterface*, double)));
    //connect(uas, SIGNAL(waypointUpdated(int,int,double,double,double,double,bool,bool)), this, SLOT(setWaypoint(int,int,double,double,double,double,bool,bool)));
    connect(uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int,int)));
    connect(&(uas->getWaypointManager()), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointUpdated(quint16)));
    connect(uas, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
    connect(UASManager::instance(), SIGNAL(activeUASStatusChanged(UASInterface*,bool)), this, SLOT(updateActiveUAS(UASInterface*,bool)));
    
    // Setup UAS selection
    connect(m_ui->uasViewFrame, SIGNAL(clicked(bool)), this, SLOT(setUASasActive(bool)));
    
    // Setup user interaction
    connect(m_ui->liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
    connect(m_ui->haltButton, SIGNAL(clicked()), uas, SLOT(halt()));
    connect(m_ui->continueButton, SIGNAL(clicked()), uas, SLOT(go()));
    connect(m_ui->landButton, SIGNAL(clicked()), uas, SLOT(home()));
    connect(m_ui->abortButton, SIGNAL(clicked()), uas, SLOT(emergencySTOP()));
    connect(m_ui->killButton, SIGNAL(clicked()), uas, SLOT(emergencyKILL()));
    connect(m_ui->shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
    
    // Set static values
    
    // Name
    if (uas->getUASName() == "")
    {
        m_ui->nameLabel->setText(tr("UAS") + QString::number(uas->getUASID()));
    }
    else
    {
        m_ui->nameLabel->setText(uas->getUASName());
    }
    
    setBackgroundColor();
    
    // Heartbeat fade
    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));

    // Hide kill and shutdown buttons per default
    m_ui->killButton->hide();
    m_ui->shutdownButton->hide();

    if (localFrame)
    {
        m_ui->gpsLabel->hide();
    }
    else
    {
        m_ui->positionLabel->hide();
    }
}

UASView::~UASView()
{
    delete m_ui;
}

/**
 * Set the background color based on the MAV color. If the MAV is selected as the
 * currently actively controlled system, the frame color is highlighted
 */
void UASView::setBackgroundColor()
{
    // UAS color
    QColor uasColor = uas->getColor();
    QString colorstyle;
    QString borderColor = "#4A4A4F";
    if (isActive)
    {
        borderColor = "#FA4A4F";
        uasColor = uasColor.darker(475);
    }
    else
    {
        uasColor = uasColor.darker(675);
    }
    colorstyle = colorstyle.sprintf("QGroupBox { border-radius: 12px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X; border: 2px solid %s; }",
                                    uasColor.red(), uasColor.green(), uasColor.blue(), borderColor.toStdString().c_str());
    m_ui->uasViewFrame->setStyleSheet(colorstyle);
}

void UASView::setUASasActive(bool active)
{
    if (active)
    {
        UASManager::instance()->setActiveUAS(this->uas);
    }
}

void UASView::updateActiveUAS(UASInterface* uas, bool active)
{
    if (uas == this->uas)
    {
        this->isActive = active;
        setBackgroundColor();
    }
}

void UASView::updateMode(int sysId, QString status, QString description)
{
    Q_UNUSED(description);
    if (sysId == this->uas->getUASID()) m_ui->modeLabel->setText(status);
}

void UASView::mouseDoubleClickEvent (QMouseEvent * event)
{
    Q_UNUSED(event);
    UASManager::instance()->setActiveUAS(uas);
    qDebug() << __FILE__ << __LINE__ << "DOUBLECLICKED";
}

void UASView::enterEvent(QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        emit uasInFocus(uas);
        if (uas != UASManager::instance()->getActiveUAS())
        {
            grabMouse(QCursor(Qt::PointingHandCursor));
        }
    }
    qDebug() << __FILE__ << __LINE__ << "IN FOCUS";
    
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        qDebug() << __FILE__ << __LINE__ << "UAS CLICKED!";
    }
}

void UASView::leaveEvent(QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        emit uasOutFocus(uas);
        releaseMouse();
    }
}

void UASView::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    refreshTimer->start(updateInterval);
}

void UASView::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    refreshTimer->stop();
}

void UASView::receiveHeartbeat(UASInterface* uas)
{
    Q_UNUSED(uas);
    QString colorstyle;
    heartbeatColor = QColor(20, 200, 20);
    colorstyle = colorstyle.sprintf("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X;}",
                                    heartbeatColor.red(), heartbeatColor.green(), heartbeatColor.blue());
    m_ui->heartbeatIcon->setStyleSheet(colorstyle);
    m_ui->heartbeatIcon->setAutoFillBackground(true);
}

/**
 * The current system type is represented through the system icon.
 *
 * @param uas Source system, has to be the same as this->uas
 * @param systemType type ID, following the MAVLink system type conventions
 * @see http://pixhawk.ethz.ch/software/mavlink
 */
void UASView::setSystemType(UASInterface* uas, unsigned int systemType)
{
    if (uas == this->uas)
    {
        // Set matching icon
        switch (systemType)
        {
        case 0:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/generic.svg"));
            break;
        case 1:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/fixed-wing.svg"));
            break;
        case 2:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/quadrotor.svg"));
            break;
        case 3:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/coaxial.svg"));
            break;
        case 4:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/helicopter.svg"));
            break;
        case 5:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/unknown.svg"));
            break;
        case 6:
            {
                // A groundstation is a special system type, update widget
                QString result;
                m_ui->nameLabel->setText(tr("OCU ") + result.sprintf("%03d", uas->getUASID()));
                m_ui->waypointLabel->setText("");
                m_ui->timeRemainingLabel->setText("Online:");
                m_ui->batteryBar->hide();
                m_ui->thrustBar->hide();
                m_ui->stateLabel->hide();
                m_ui->statusTextLabel->hide();
                m_ui->waypointLabel->hide();
                m_ui->liftoffButton->hide();
                m_ui->haltButton->hide();
                m_ui->landButton->hide();
                m_ui->shutdownButton->hide();
                m_ui->abortButton->hide();
                m_ui->typeButton->setIcon(QIcon(":/images/mavs/groundstation.svg"));
            }
            break;
        default:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/unknown.svg"));
            break;
        }
    }
}

void UASView::updateLocalPosition(UASInterface* uas, double x, double y, double z, quint64 usec)
{
    Q_UNUSED(usec);
    Q_UNUSED(uas);
    this->x = x;
    this->y = y;
    this->z = z;
    if (!localFrame)
    {
        localFrame = true;
        m_ui->gpsLabel->hide();
        m_ui->positionLabel->show();
    }
}

void UASView::updateGlobalPosition(UASInterface* uas, double lon, double lat, double alt, quint64 usec)
{
    Q_UNUSED(uas);
    Q_UNUSED(usec);
    this->lon = lon;
    this->lat = lat;
    this->alt = alt;
}

void UASView::updateSpeed(UASInterface*, double x, double y, double z, quint64 usec)
{
    Q_UNUSED(usec);
    totalSpeed = sqrt((pow(x, 2) + pow(y, 2) + pow(z, 2)));
}

void UASView::currentWaypointUpdated(quint16 waypoint)
{
    m_ui->waypointLabel->setText(tr("WP") + QString::number(waypoint));
}

void UASView::setWaypoint(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool current)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
    Q_UNUSED(yaw);
    Q_UNUSED(autocontinue);
    if (uasId == this->uas->getUASID())
    {
        if (current)
        {
            m_ui->waypointLabel->setText(tr("WP") + QString::number(id));
        }
    }
}

void UASView::selectWaypoint(int uasId, int id)
{
    if (uasId == this->uas->getUASID())
    {
        m_ui->waypointLabel->setText(tr("WP") + QString::number(id));
    }
}

void UASView::updateThrust(UASInterface* uas, double thrust)
{
    if (this->uas == uas)
    {
        this->thrust = thrust;
    }
}

void UASView::updateBattery(UASInterface* uas, double voltage, double percent, int seconds)
{
    Q_UNUSED(voltage);
    if (this->uas == uas)
    {
        timeRemaining = seconds;
        chargeLevel = percent;
    }
}

void UASView::updateState(UASInterface* uas, QString uasState, QString stateDescription)
{
    if (this->uas == uas)
    {
        state = uasState;
        stateDesc = stateDescription;
    }
}

void UASView::updateLoad(UASInterface* uas, double load)
{
    if (this->uas == uas)
    {
        this->load = load;
    }
}

void UASView::refresh()
{
    //setUpdatesEnabled(false);
    //setUpdatesEnabled(true);
    //repaint();

    static quint64 lastupdate = 0;
    //qDebug() << "UASVIEW update diff: " << MG::TIME::getGroundTimeNow() - lastupdate;
    lastupdate = MG::TIME::getGroundTimeNow();

    // FIXME
    static int generalUpdateCount = 0;

    if (generalUpdateCount == 4)
    {
#if (QGC_EVENTLOOP_DEBUG)
        qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif
        generalUpdateCount = 0;
        //qDebug() << "UPDATING EVERYTHING";
        // State
        m_ui->stateLabel->setText(state);
        m_ui->statusTextLabel->setText(stateDesc);

        // Battery
        m_ui->batteryBar->setValue(static_cast<int>(this->chargeLevel));
        //m_ui->loadBar->setValue(static_cast<int>(this->load));
        m_ui->thrustBar->setValue(this->thrust);

        // Position
        QString position;
        position = position.sprintf("%05.1f %05.1f %05.1f m", x, y, z);
        m_ui->positionLabel->setText(position);
        QString globalPosition;
        QString latIndicator;
        if (lat > 0)
        {
            latIndicator = "N";
        }
        else
        {
            latIndicator = "S";
        }
        QString lonIndicator;
        if (lon > 0)
        {
            lonIndicator = "E";
        }
        else
        {
            lonIndicator = "W";
        }
        globalPosition = globalPosition.sprintf("%05.1f%s %05.1f%s %05.1f m", lon, lonIndicator.toStdString().c_str(), lat, latIndicator.toStdString().c_str(), alt);
        m_ui->gpsLabel->setText(globalPosition);

        // Altitude
        if (groundDistance == 0 && alt != 0)
        {
            m_ui->groundDistanceLabel->setText(QString("%1 m").arg(alt, 5, 'f', 1, '0'));
        }
        else
        {
            m_ui->groundDistanceLabel->setText(QString("%1 m").arg(groundDistance, 5, 'f', 1, '0'));
        }

        // Speed
        QString speed("%1 m/s");
        m_ui->speedLabel->setText(speed.arg(totalSpeed, 4, 'f', 1, '0'));

        // Thrust
        m_ui->thrustBar->setValue(thrust * 100);

        if(this->timeRemaining > 1 && this->timeRemaining < MG::MAX_FLIGHT_TIME)
        {
            // Filter output to get a higher stability
            static double filterTime = static_cast<int>(this->timeRemaining);
            filterTime = 0.8 * filterTime + 0.2 * static_cast<int>(this->timeRemaining);
            int sec = static_cast<int>(filterTime - static_cast<int>(filterTime / 60.0f) * 60);
            int min = static_cast<int>(filterTime / 60);
            int hours = static_cast<int>(filterTime - min * 60 - sec);

            QString timeText;
            timeText = timeText.sprintf("%02d:%02d:%02d", hours, min, sec);
            m_ui->timeRemainingLabel->setText(timeText);
        }
        else
        {
            m_ui->timeRemainingLabel->setText(tr("Calc.."));
        }

        // Time Elapsed
        //QDateTime time = MG::TIME::msecToQDateTime(uas->getUptime());

        quint64 filterTime = uas->getUptime() / 1000;
        int sec = static_cast<int>(filterTime - static_cast<int>(filterTime / 60) * 60);
        int min = static_cast<int>(filterTime / 60);
        int hours = static_cast<int>(filterTime - min * 60 - sec);
        QString timeText;
        timeText = timeText.sprintf("%02d:%02d:%02d", hours, min, sec);
        m_ui->timeElapsedLabel->setText(timeText);
    }
    generalUpdateCount++;

    // Fade heartbeat icon
    // Make color darker
    heartbeatColor = heartbeatColor.darker(150);

    QString colorstyle;
    colorstyle = colorstyle.sprintf("QGroupBox { border: 1px solid #EEEEEE; border-radius: 8px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X;}",
                                    heartbeatColor.red(), heartbeatColor.green(), heartbeatColor.blue());
    m_ui->heartbeatIcon->setStyleSheet(colorstyle);
    m_ui->heartbeatIcon->setAutoFillBackground(true);
    //setUpdatesEnabled(true);

    //setUpdatesEnabled(false);
}

void UASView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

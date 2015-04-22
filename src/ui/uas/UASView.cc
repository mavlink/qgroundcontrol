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
#include <QMenu>
#include <QInputDialog>

#include "QGC.h"
#include "UASManager.h"
#include "UASView.h"
#include "UASWaypointManager.h"
#include "MainWindow.h"
#include "ui_UASView.h"

#ifndef __mobile__
#include <QGCHilFlightGearConfiguration.h>
#endif

UASView::UASView(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    uas(uas),
    startTime(0),
    timeout(false),
    iconIsRed(true),
    disconnected(false),
    timeRemaining(0),
    chargeLevel(0),
    load(0),
    state("UNKNOWN"),
    stateDesc(tr("Unknown state")),
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
    globalFrameKnown(false),
    removeAction(new QAction(tr("Delete this system"), this)),
    renameAction(new QAction(tr("Rename.."), this)),
    selectAction(new QAction(tr("Control this system"), this)),
    hilAction(new QAction(tr("HIL - Hardware in the Loop"), this)),
    selectAirframeAction(new QAction(tr("Choose Airframe"), this)),
    setBatterySpecsAction(new QAction(tr("Set Battery Options"), this)),
    lowPowerModeEnabled(true),
    generalUpdateCount(0),
    filterTime(0),
    m_ui(new Ui::UASView)
{
    m_ui->setupUi(this);
    setToolTip(""); // Make sure the QGroupBox's tooltip doesn't seep through.

    // FIXME XXX
    lowPowerModeEnabled = MainWindow::instance()->lowPowerModeEnabled();

    hilAction->setCheckable(true);

    // Setup communication
    //connect(uas, SIGNAL(valueChanged(int,QString,double,quint64)), this, SLOT(receiveValue(int,QString,double,quint64)));
    connect(uas, SIGNAL(batteryChanged(UASInterface*, double, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, double, int)));
    connect(uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));
    connect(uas, SIGNAL(thrustChanged(UASInterface*, double)), this, SLOT(updateThrust(UASInterface*, double)));
    connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,double,quint64)));
    connect(uas, SIGNAL(velocityChanged_NED(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString,QString)));
    connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
    connect(uas, SIGNAL(loadChanged(UASInterface*, double)), this, SLOT(updateLoad(UASInterface*, double)));
    connect(uas, SIGNAL(heartbeatTimeout(bool, unsigned int)), this, SLOT(heartbeatTimeout(bool, unsigned int)));
    connect(uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int,int)));
    connect(uas->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointUpdated(quint16)));
    connect(uas, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
    connect(UASManager::instance(), SIGNAL(activeUASStatusChanged(UASInterface*,bool)), this, SLOT(updateActiveUAS(UASInterface*,bool)));
    connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(showStatusText(int, int, int, QString)));
    connect(uas, SIGNAL(navModeChanged(int, int, QString)), this, SLOT(updateNavMode(int, int, QString)));

    // Setup user interaction
    connect(m_ui->liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
    connect(m_ui->haltButton, SIGNAL(clicked()), uas, SLOT(halt()));
    connect(m_ui->continueButton, SIGNAL(clicked()), uas, SLOT(go()));
    connect(m_ui->landButton, SIGNAL(clicked()), uas, SLOT(land()));
    connect(m_ui->abortButton, SIGNAL(clicked()), uas, SLOT(emergencySTOP()));
    connect(m_ui->killButton, SIGNAL(clicked()), uas, SLOT(emergencyKILL()));
    connect(m_ui->shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));

    // Allow deleting this widget
    connect(removeAction, SIGNAL(triggered()), this, SLOT(triggerUASDeletion()));
    connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));
    connect(selectAction, SIGNAL(triggered()), uas, SLOT(setSelected()));
    connect(selectAirframeAction, SIGNAL(triggered()), this, SLOT(selectAirframe()));
    connect(setBatterySpecsAction, SIGNAL(triggered()), this, SLOT(setBatterySpecs()));

    // Name changes
    connect(uas, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));

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

    // Heartbeat fade
    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    if (lowPowerModeEnabled)
    {
        refreshTimer->start(updateInterval*3);
    }
    else
    {
        refreshTimer->start(updateInterval);
    }

    // Style some elements by default to the UAS' color.
    heartbeatColor = uas->getColor();
    QString colorstyle("QLabel { background-color: %1; }");
    m_ui->typeLabel->setStyleSheet(colorstyle.arg(heartbeatColor.name()));
    updateActiveUAS(uas, false);

    // Hide kill and shutdown buttons by default
    m_ui->killButton->hide();
    m_ui->shutdownButton->hide();

    // Set state and mode
    updateMode(uas->getUASID(), uas->getShortMode(), "");
    updateState(uas, uas->getShortState(), "");
    setSystemType(uas, uas->getSystemType());
}

UASView::~UASView()
{
    delete m_ui;
}

void UASView::heartbeatTimeout(bool timeout, unsigned int ms)
{
    Q_UNUSED(ms);
    this->timeout = timeout;
}

void UASView::updateNavMode(int uasid, int mode, const QString& text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(mode);
    m_ui->navLabel->setText(text);
}

void UASView::showStatusText(int uasid, int componentid, int severity, QString text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(componentid);
    Q_UNUSED(severity);
    //m_ui->statusTextLabel->setText(text);
    stateDesc = text;
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
        if (active)
        {
            setStyleSheet("UASView { border-width: 3px}");
        }
        else
        {
            setStyleSheet(QString("UASView { border-color: %1}").arg(heartbeatColor.name()));
        }
    }
}

void UASView::updateMode(int sysId, QString status, QString description)
{
    Q_UNUSED(description);

    //int aa=this->uas->getUASID();
    if (sysId == this->uas->getUASID()) m_ui->modeLabel->setText(status);

    m_ui->modeLabel->setText(status);
}

void UASView::mouseDoubleClickEvent (QMouseEvent * event)
{
    Q_UNUSED(event);
    UASManager::instance()->setActiveUAS(uas);
}

void UASView::receiveHeartbeat(UASInterface* uas)
{
    heartbeatColor = uas->getColor();
    QString colorstyle("QLabel { background-color: %1; }");
    m_ui->heartBeatLabel->setStyleSheet(colorstyle.arg(heartbeatColor.name()));

    // If we're returning from a disconnection, recolor things properly.
    if (disconnected)
    {
        updateActiveUAS(this->uas, this->isActive);
        disconnected = false;
    }
    timeout = false;
}

void UASView::updateName(const QString& name)
{
    if (uas) m_ui->nameLabel->setText(name);
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
        case MAV_TYPE_GENERIC:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Generic"));
            break;
        case MAV_TYPE_FIXED_WING:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/FixedWing"));
            break;
        case MAV_TYPE_QUADROTOR:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/QuadRotor"));
            break;
        case MAV_TYPE_COAXIAL:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Coaxial"));
            break;
        case MAV_TYPE_HELICOPTER:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Helicopter"));
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Unknown"));
            break;
        case MAV_TYPE_GCS: {
                // A groundstation is a special system type, update widget
                QString result;
                m_ui->nameLabel->setText(tr("GCS ") + result.sprintf("%03d", uas->getUASID()));
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
                m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Groundstation"));
            }
            break;
        case MAV_TYPE_AIRSHIP:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Airship"));
            break;
        case MAV_TYPE_FREE_BALLOON:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/FreeBalloon"));
            break;
        case MAV_TYPE_ROCKET:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Rocket"));
            break;
        case MAV_TYPE_GROUND_ROVER:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/GroundRover"));
            break;
        case MAV_TYPE_SURFACE_BOAT:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/SurfaceBoat"));
            break;
        case MAV_TYPE_SUBMARINE:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Submarine"));
            break;
        case MAV_TYPE_HEXAROTOR:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/HexaRotor"));
            break;
        case MAV_TYPE_OCTOROTOR:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/OctoRotor"));
            break;
        case MAV_TYPE_TRICOPTER:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/TriCopter"));
            break;
        case MAV_TYPE_FLAPPING_WING:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/FlappingWing"));
            break;
        case MAV_TYPE_KITE:
            m_ui->typeLabel->setPixmap(QPixmap(":/res/mavs/Kite"));
            break;
        default:
            m_ui->typeLabel->setPixmap(QPixmap("://res/mavs/Unknown"));
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
    localFrame = true;
}

void UASView::updateGlobalPosition(UASInterface* uas, double lon, double lat, double altAMSL, double altWGS84, quint64 usec)
{
    Q_UNUSED(uas);
    Q_UNUSED(usec);
    Q_UNUSED(altAMSL);
    this->lon = lon;
    this->lat = lat;
    this->alt = altWGS84;
    globalFrameKnown = true;
}

void UASView::updateSpeed(UASInterface*, double x, double y, double z, quint64 usec)
{
    Q_UNUSED(usec);
    totalSpeed = sqrt(x*x + y*y + z*z);
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

void UASView::updateBattery(UASInterface* uas, double voltage, double current, double percent, int seconds)
{
    Q_UNUSED(voltage);
    Q_UNUSED(current);
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

/**
 * Right-clicking on the view provides a custom menu for interacting
 * with the UAS.
 */
void UASView::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(selectAction);
    menu.addSeparator();
    menu.addAction(renameAction);
/*
    FIXME: The code below is incorrect. removeAction should only be available when link is
            disconnected. fSee Issue #1275
    if (timeout)
    {
        menu.addAction(removeAction);
    }
*/
    menu.addAction(hilAction);
    menu.addAction(selectAirframeAction);
    menu.addAction(setBatterySpecsAction);
    menu.exec(event->globalPos());
}

void UASView::setBatterySpecs()
{
    if (uas)
    {
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Set Battery Specifications for %1").arg(uas->getUASName()),
                                                tr("Specs: (empty,warn,full), e.g. (9V,9.5V,12.6V) or just warn level in percent (e.g. 15%) to use estimate from MAV"), QLineEdit::Normal,
                                                uas->getBatterySpecs(), &ok);

        if (ok && !newName.isEmpty()) uas->setBatterySpecs(newName);
    }
}

void UASView::rename()
{
    if (uas)
    {
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Rename System %1").arg(uas->getUASName()),
                                                tr("System Name:"), QLineEdit::Normal,
                                                uas->getUASName(), &ok);

        if (ok && !newName.isEmpty()) uas->setUASName(newName);
    }
}

void UASView::selectAirframe()
{
    if (uas)
    {
        // Get list of airframes from UAS
        QStringList airframes;
        airframes << "Generic"
                << "Multiplex Easystar"
                << "Multiplex Twinstar"
                << "Multiplex Merlin"
                << "Pixhawk Cheetah"
                << "Mikrokopter"
                << "Reaper"
                << "Predator"
                << "Coaxial"
                << "Pteryx"
                << "Tricopter"
                << "Asctec Firefly"
                << "X8 Flying Wing"
                << "Viper 2.0 Flying Wing"
                << "Cam Flyer Q Flying Wing";

        bool ok;
        QString item = QInputDialog::getItem(this, tr("Select Airframe for %1").arg(uas->getUASName()),
                                             tr("Airframe"), airframes, uas->getAirframe(), false, &ok);
        if (ok && !item.isEmpty())
        {
            // Set this airframe as UAS airframe
            uas->setAirframe(airframes.indexOf(item));
        }
    }
}

void UASView::triggerUASDeletion()
{
    refreshTimer->stop();
    UASManager::instance()->removeUAS(uas);
}

void UASView::refresh()
{
    if (generalUpdateCount == 4)
    {
#if (QGC_EVENTLOOP_DEBUG)
        // qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif
        generalUpdateCount = 0;
        //// qDebug() << "UPDATING EVERYTHING";
        // State
        m_ui->stateLabel->setText(state);
        m_ui->statusTextLabel->setText(stateDesc);

        // Battery
        m_ui->batteryBar->setValue(static_cast<int>(this->chargeLevel));
        //m_ui->loadBar->setValue(static_cast<int>(this->load));
        m_ui->thrustBar->setValue(this->thrust);

        // Position
        // If global position is known, prefer it over local coordinates

        if (!globalFrameKnown && localFrame)
        {
            QString position;
            position = position.sprintf("%05.1f %05.1f %06.1f m", x, y, z);
            m_ui->positionLabel->setText(position);
        }

        if (globalFrameKnown)
        {
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

            globalPosition = globalPosition.sprintf("%05.1f%s %05.1f%s %06.1f m", lon, lonIndicator.toStdString().c_str(), lat, latIndicator.toStdString().c_str(), alt);
            m_ui->positionLabel->setText(globalPosition);
        }

        // Altitude
        if (groundDistance == 0 && alt != 0)
        {
            m_ui->groundDistanceLabel->setText(QString("%1 m").arg(alt, 6, 'f', 1, '0'));
        }
        else
        {
            m_ui->groundDistanceLabel->setText(QString("%1 m").arg(groundDistance, 6, 'f', 1, '0'));
        }

        // Speed
        QString speed("%1 m/s");
        m_ui->speedLabel->setText(speed.arg(totalSpeed, 4, 'f', 1, '0'));

        // Thrust
        m_ui->thrustBar->setValue(thrust * 100);

        // Time Elapsed
        //QDateTime time = MG::TIME::msecToQDateTime(uas->getUptime());

       quint64 filterTime = uas->getUptime() / 1000;
       int hours = static_cast<int>(filterTime / 3600);
       int min = static_cast<int>((filterTime - 3600 * hours) / 60);
       int sec = static_cast<int>(filterTime - 60 * min - 3600 * hours);
       QString timeText;
       timeText = timeText.sprintf("%02d:%02d:%02d", hours, min, sec);
       m_ui->timeElapsedLabel->setText(timeText);

        if(this->timeRemaining > 1 && this->timeRemaining < QGC::MAX_FLIGHT_TIME)
        {
            // Filter output to get a higher stability
            filterTime = static_cast<int>(this->timeRemaining);
            // filterTime = 0.8 * filterTime + 0.2 * static_cast<int>(this->timeRemaining);
            hours = static_cast<int>(filterTime / 3600);
            min = static_cast<int>((filterTime - 3600 * hours) / 60);
            sec = static_cast<int>(filterTime - 60 * min - 3600 * hours);

            timeText = timeText.sprintf("%02d:%02d:%02d", hours, min, sec);
            m_ui->timeRemainingLabel->setText(timeText);
        }
        else
        {
            m_ui->timeRemainingLabel->setText(tr("Calc.."));
        }


    }
    generalUpdateCount++;

    if (timeout)
    {
        // CRITICAL CONDITION, NO HEARTBEAT
        disconnected = true;

        QColor warnColor;
        if (iconIsRed)
        {
            warnColor = Qt::red;
        }
        else
        {
            warnColor = Qt::darkRed;
            refreshTimer->setInterval(errorUpdateInterval);
            refreshTimer->start();
        }
        QString style = QString("UASView {background-color: %1;}").arg(warnColor.name());
        this->setStyleSheet(style);
        iconIsRed = !iconIsRed;
    }
    else
    {
        // If we're not in low power mode, add the additional visual effect of
        // fading out the color of the heartbeat for this UAS.
        if (!lowPowerModeEnabled)
        {
            heartbeatColor = heartbeatColor.darker(110);
            QString colorstyle("QLabel {background-color: %1;}");
            m_ui->heartBeatLabel->setStyleSheet(colorstyle.arg(heartbeatColor.name()));
            refreshTimer->setInterval(updateInterval);
            refreshTimer->start();
        }
    }
}

void UASView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void UASView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

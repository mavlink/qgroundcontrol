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

#include "MG.h"
#include "UASManager.h"
#include "UASView.h"
#include "ui_UASView.h"

UASView::UASView(UASInterface* uas, QWidget *parent) :
        QWidget(parent),
        timeRemaining(0),
        state("UNKNOWN"),
        stateDesc(tr("Unknown system state")),
        mode("MAV_MODE_UNKNOWN"),
        thrust(0),
        m_ui(new Ui::UASView)
{
    this->uas = uas;

    m_ui->setupUi(this);

    // Setup communication
    connect(uas, SIGNAL(valueChanged(int,QString,double,quint64)), this, SLOT(receiveValue(int,QString,double,quint64)));
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
    connect(uas, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));

    // Setup UAS selection
    connect(m_ui->groupBox, SIGNAL(clicked(bool)), this, SLOT(setUASasActive(bool)));

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

    // Get min/max values from UAS
    // TODO get these values from UAS
    //m_ui->speedBar->setMinimum(0);
    //m_ui->speedBar->setMaximum(15);

    // UAS color
    QColor uasColor = uas->getColor();
    uasColor = uasColor.darker(475);
    QString colorstyle;
    colorstyle = colorstyle.sprintf("QGroupBox { border: 2px solid #4A4A4F; border-radius: 5px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X;}",
                                    uasColor.red(), uasColor.green(), uasColor.blue());
    m_ui->groupBox->setStyleSheet(colorstyle);
    //m_ui->groupBox->setAutoFillBackground(true);


    // Heartbeat fade
    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    refreshTimer->start(100);
}

UASView::~UASView()
{
    delete m_ui;
}

void UASView::setUASasActive(bool)
{
    UASManager::instance()->setActiveUAS(this->uas);
}

void UASView::updateMode(int sysId, QString status, QString description)
{
    if (sysId == this->uas->getUASID()) m_ui->modeLabel->setText(status);
}

void UASView::mouseDoubleClickEvent (QMouseEvent * event)
{
    UASManager::instance()->setActiveUAS(uas);
    qDebug() << __FILE__ << __LINE__ << "DOUBLECLICKED";
}

void UASView::enterEvent(QEvent* event)
{
    if (event->MouseMove) emit uasInFocus(uas);
    qDebug() << __FILE__ << __LINE__ << "IN FOCUS";
}

void UASView::leaveEvent(QEvent* event)
{
    if (event->MouseMove) emit uasOutFocus(uas);
    qDebug() << __FILE__ << __LINE__ << "OUT OF FOCUS";
}

void UASView::receiveHeartbeat(UASInterface* uas)
{
    if (uas == this->uas)
    {
        refreshTimer->stop();
        QString colorstyle;
        heartbeatColor = QColor(20, 200, 20);
        colorstyle = colorstyle.sprintf("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X;}",
                                        heartbeatColor.red(), heartbeatColor.green(), heartbeatColor.blue());
        m_ui->heartbeatIcon->setStyleSheet(colorstyle);
        m_ui->heartbeatIcon->setAutoFillBackground(true);
        refreshTimer->start(50);
    }
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
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/groundstation.svg"));
            break;
        default:
            m_ui->typeButton->setIcon(QIcon(":/images/mavs/unknown.svg"));
            break;
        }
    }
}

void UASView::updateLocalPosition(UASInterface* uas, double x, double y, double z, quint64 usec)
{
    if (uas == this->uas)
    {
        QString position;
        position = position.sprintf("%02.2f %02.2f %02.2f m", x, y, z);
        m_ui->positionLabel->setText(position);
    }
}

void UASView::updateGlobalPosition(UASInterface*, double lon, double lat, double alt, quint64 usec)
{
}

void UASView::updateSpeed(UASInterface*, double x, double y, double z, quint64 usec)
{
    //    double totalSpeed = sqrt((pow(x, 2) + pow(y, 2) + pow(z, 2)));
    //    m_ui->speedBar->setValue(totalSpeed);
}

void UASView::receiveValue(int uasid, QString id, double value, quint64 time)
{

}

void UASView::setWaypoint(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool current)
{
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
        m_ui->thrustBar->setValue(thrust * 100);
    }
}

void UASView::updateBattery(UASInterface* uas, double voltage, double percent, int seconds)
{
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
    // State
    m_ui->stateLabel->setText(state);
    m_ui->statusTextLabel->setText(stateDesc);

    // Battery
    m_ui->batteryBar->setValue(static_cast<int>(this->chargeLevel));
    //m_ui->loadBar->setValue(static_cast<int>(this->load));
    m_ui->thrustBar->setValue(this->thrust);

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
        m_ui->timeRemainingLabel->setText(tr("Calculating"));
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

    // Fade heartbeat icon
    // Make color darker
    heartbeatColor = heartbeatColor.darker(110);

    QString colorstyle;
    colorstyle = colorstyle.sprintf("QGroupBox { border: 1px solid #EEEEEE; border-radius: 4px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X;}",
                                    heartbeatColor.red(), heartbeatColor.green(), heartbeatColor.blue());
    m_ui->heartbeatIcon->setStyleSheet(colorstyle);
    m_ui->heartbeatIcon->setAutoFillBackground(true);
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

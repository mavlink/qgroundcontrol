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
 *   @brief Waypoint list widget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include "WaypointList.h"
#include "ui_WaypointList.h"
#include <UASInterface.h>
#include <UASManager.h>
#include <QDebug>
#include <QFileDialog>

WaypointList::WaypointList(QWidget *parent, UASInterface* uas) :
        QWidget(parent),
        uas(NULL),
        m_ui(new Ui::WaypointList)
{
    m_ui->setupUi(this);

    transmitDelay = new QTimer(this);

    listLayout = new QVBoxLayout(m_ui->listWidget);
    listLayout->setSpacing(6);
    listLayout->setMargin(0);
    listLayout->setAlignment(Qt::AlignTop);
    m_ui->listWidget->setLayout(listLayout);

    wpViews = QMap<Waypoint*, WaypointView*>();

    this->uas = NULL;

    // ADD WAYPOINT
    // Connect add action, set right button icon and connect action to this class
    connect(m_ui->addButton, SIGNAL(clicked()), m_ui->actionAddWaypoint, SIGNAL(triggered()));
    connect(m_ui->actionAddWaypoint, SIGNAL(triggered()), this, SLOT(add()));

    // SEND WAYPOINTS
    connect(m_ui->transmitButton, SIGNAL(clicked()), this, SLOT(transmit()));

    // REQUEST WAYPOINTS
    connect(m_ui->readButton, SIGNAL(clicked()), this, SLOT(read()));

    // SAVE/LOAD WAYPOINTS
    connect(m_ui->saveButton, SIGNAL(clicked()), this, SLOT(saveWaypoints()));
    connect(m_ui->loadButton, SIGNAL(clicked()), this, SLOT(loadWaypoints()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    connect(transmitDelay, SIGNAL(timeout()), this, SLOT(reenableTransmit()));

    // STATUS LABEL
    updateStatusLabel("");

    // SET UAS AFTER ALL SIGNALS/SLOTS ARE CONNECTED
    setUAS(uas);
}

WaypointList::~WaypointList()
{
    delete m_ui;
}

void WaypointList::updateStatusLabel(const QString &string)
{
    m_ui->statusLabel->setText(string);
}

void WaypointList::setUAS(UASInterface* uas)
{
    if (this->uas == NULL && uas != NULL)
    {
        this->uas = uas;
        connect(&uas->getWaypointManager(), SIGNAL(waypointUpdated(int,quint16,double,double,double,double,bool,bool)), this, SLOT(setWaypoint(int,quint16,double,double,double,double,bool,bool)));
        //connect(this, SIGNAL(waypointChanged(Waypoint*)),   &uas->getWaypointManager(), SLOT(setWaypoint(Waypoint*)));
        //connect(this, SIGNAL(currentWaypointChanged(int)),  &uas->getWaypointManager(), SLOT(setWaypointActive(quint16)));
        connect(this, SIGNAL(sendWaypoints(const QVector<Waypoint*> &)),    &uas->getWaypointManager(), SLOT(sendWaypoints(const QVector<Waypoint*> &)));
        connect(this, SIGNAL(requestWaypoints()),                           &uas->getWaypointManager(), SLOT(requestWaypoints()));
        connect(this, SIGNAL(clearWaypointList()),                          &uas->getWaypointManager(), SLOT(clearWaypointList()));

        connect(&uas->getWaypointManager(), SIGNAL(updateStatusString(const QString &)), this, SLOT(updateStatusLabel(const QString &)));
    }
}

void WaypointList::setWaypoint(int uasId, quint16 id, double x, double y, double z, double yaw, bool autocontinue, bool current)
{
    if (uasId == this->uas->getUASID())
    {
        transmitDelay->start(1000);
        Waypoint* wp = new Waypoint(id, x, y, z, yaw, autocontinue, current);
        addWaypoint(wp);
    }
}

void WaypointList::waypointReached(UASInterface* uas, quint16 waypointId)
{
    Q_UNUSED(uas);
    qDebug() << "Waypoint reached: " << waypointId;

    /*if (waypoints.size() > waypointId)
    {
        if (waypoints[waypointId]->autocontinue == true)
        {

            for(int i = 0; i < waypoints.size(); i++)
            {
                if (i == waypointId+1)
                {
                    waypoints[i]->current = true;
                    WaypointView* widget = wpViews.find(waypoints[i]).value();
                    widget->setCurrent();
                }
                else
                {
                    if (waypoints[i]->current)
                    {
                        waypoints[i]->current = false;
                        WaypointView* widget = wpViews.find(waypoints[i]).value();
                        widget->removeCurrentCheck();
                    }
                }
            }
            redrawList();

            qDebug() << "NEW WAYPOINT SET";
        }
    }*/
}

void WaypointList::read()
{
    while(waypoints.size()>0)
    {
        removeWaypoint(waypoints[0]);
    }

    emit requestWaypoints();
}

void WaypointList::transmit()
{
    transmitDelay->start(1000);
    m_ui->transmitButton->setEnabled(false);

    emit sendWaypoints(waypoints);
    //emit requestWaypoints(); FIXME
}

void WaypointList::add()
{
    // Only add waypoints if UAS is present
    if (uas)
    {
        if (waypoints.size() > 0)
        {
            addWaypoint(new Waypoint(waypoints.size(), 0.0, 0.0, -0.0, 0.0, false, false));
        }
        else
        {
            addWaypoint(new Waypoint(waypoints.size(), 0.0, 0.0, -0.0, 0.0, false, true));
        }
    }
}

void WaypointList::addWaypoint(Waypoint* wp)
{
    waypoints.push_back(wp);
    if (!wpViews.contains(wp))
    {
        WaypointView* wpview = new WaypointView(wp, this);
        wpViews.insert(wp, wpview);
        listLayout->addWidget(wpViews.value(wp));
        connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)), this, SLOT(moveDown(Waypoint*)));
        connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)), this, SLOT(moveUp(Waypoint*)));
        connect(wpview, SIGNAL(removeWaypoint(Waypoint*)), this, SLOT(removeWaypoint(Waypoint*)));
        connect(wpview, SIGNAL(setCurrentWaypoint(Waypoint*)), this, SLOT(setCurrentWaypoint(Waypoint*)));
        //connect(wpview, SIGNAL(waypointUpdated(Waypoint*)), this, SIGNAL(waypointChanged(Waypoint*)));
    }
}

void WaypointList::redrawList()
{
    // Clear list layout
    if (!wpViews.empty())
    {
        QMapIterator<Waypoint*,WaypointView*> viewIt(wpViews);
        viewIt.toFront();
        while(viewIt.hasNext())
        {
            viewIt.next();
            listLayout->removeWidget(viewIt.value());
        }
        // Re-add waypoints
        for(int i = 0; i < waypoints.size(); i++)
        {
            listLayout->addWidget(wpViews.value(waypoints[i]));
        }
    }
}

void WaypointList::moveUp(Waypoint* wp)
{
    int id = wp->getId();
    if (waypoints.size() > 1 && waypoints.size() > id)
    {
        Waypoint* temp = waypoints[id];
        if (id > 0)
        {
            waypoints[id] = waypoints[id-1];
            waypoints[id-1] = temp;
            waypoints[id-1]->setId(id-1);
            waypoints[id]->setId(id);
        }
        else
        {
            waypoints[id] = waypoints[waypoints.size()-1];
            waypoints[waypoints.size()-1] = temp;
            waypoints[waypoints.size()-1]->setId(waypoints.size()-1);
            waypoints[id]->setId(id);
        }
        redrawList();
    }
}

void WaypointList::moveDown(Waypoint* wp)
{
    int id = wp->getId();
    if (waypoints.size() > 1 && waypoints.size() > id)
    {
        Waypoint* temp = waypoints[id];
        if (id != waypoints.size()-1)
        {
            waypoints[id] = waypoints[id+1];
            waypoints[id+1] = temp;
            waypoints[id+1]->setId(id+1);
            waypoints[id]->setId(id);
        }
        else
        {
            waypoints[id] = waypoints[0];
            waypoints[0] = temp;
            waypoints[0]->setId(0);
            waypoints[id]->setId(id);
        }
        redrawList();
    }
}

void WaypointList::removeWaypoint(Waypoint* wp)
{
    // Delete from list
    if (wp != NULL)
    {
        waypoints.remove(wp->getId());
        for(int i = wp->getId(); i < waypoints.size(); i++)
        {
            waypoints[i]->setId(i);
        }

        // Remove from view
        WaypointView* widget = wpViews.find(wp).value();
        wpViews.remove(wp);
        widget->hide();
        listLayout->removeWidget(widget);
        delete wp;
    }
}

void WaypointList::setCurrentWaypoint(Waypoint* wp)
{
    for(int i = 0; i < waypoints.size(); i++)
    {
        if (waypoints[i] == wp)
        {
            waypoints[i]->setCurrent(true);
            // Retransmit waypoint
            //uas->getWaypointManager().setWaypointActive(i);
        }
        else
        {
            if (waypoints[i]->getCurrent())
            {
                waypoints[i]->setCurrent(false);
                WaypointView* widget = wpViews.find(waypoints[i]).value();
                widget->removeCurrentCheck();
            }
        }
    }
}

void WaypointList::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void WaypointList::reenableTransmit()
{
    m_ui->transmitButton->setEnabled(true);
}

void WaypointList::saveWaypoints()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "./waypoints.txt", tr("Waypoint File (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    for (int i = 0; i < waypoints.size(); i++)
    {
        Waypoint* wp = waypoints[i];
        in << "~" << wp->getId() << "~" << wp->getX() << "~" << wp->getY()  << "~" << wp->getZ()  << "~" << wp->getYaw()  << "~" << wp->getAutoContinue() << "~" << wp->getCurrent() << "\n";
        in.flush();
    }
    file.close();
}

void WaypointList::loadWaypoints()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Waypoint File (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while(waypoints.size()>0)
    {
        removeWaypoint(waypoints[0]);
    }

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QStringList wpParams = in.readLine().split("~");
        if (wpParams.size() == 8)
            addWaypoint(new Waypoint(wpParams[1].toInt(), wpParams[2].toDouble(), wpParams[3].toDouble(), wpParams[4].toDouble(), wpParams[5].toDouble(), (wpParams[6].toInt() == 1 ? true : false), (wpParams[7].toInt() == 1 ? true : false)));
    }
    file.close();
}


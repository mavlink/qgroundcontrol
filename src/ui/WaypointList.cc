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

    // SAVE/LOAD WAYPOINTS
    connect(m_ui->saveButton, SIGNAL(clicked()), this, SLOT(saveWaypoints()));
    connect(m_ui->loadButton, SIGNAL(clicked()), this, SLOT(loadWaypoints()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    connect(transmitDelay, SIGNAL(timeout()), this, SLOT(reenableTransmit()));

    // SET UAS AFTER ALL SIGNALS/SLOTS ARE CONNECTED
    setUAS(uas);
}

WaypointList::~WaypointList()
{
    delete m_ui;
}

void WaypointList::setUAS(UASInterface* uas)
{
    if (this->uas == NULL && uas != NULL)
    {
        this->uas = uas;
        connect(&uas->getWaypointManager(), SIGNAL(waypointUpdated(int,int,double,double,double,double,bool,bool)), this, SLOT(setWaypoint(int,int,double,double,double,double,bool,bool)));
        connect(&uas->getWaypointManager(), SIGNAL(waypointReached(UASInterface*,int)), this, SLOT(waypointReached(UASInterface*,int)));
        connect(this, SIGNAL(waypointChanged(Waypoint*)), &uas->getWaypointManager(), SLOT(setWaypoint(Waypoint*)));
        connect(this, SIGNAL(currentWaypointChanged(int)), &uas->getWaypointManager(), SLOT(setWaypointActive(int)));
        // This slot is not implemented in UAS: connect(this, SIGNAL(removeWaypointId(int)), uas, SLOT(removeWaypoint(Waypoint*)));
        connect(this, SIGNAL(requestWaypoints()), &uas->getWaypointManager(), SLOT(requestWaypoints()));
        connect(this, SIGNAL(clearWaypointList()), &uas->getWaypointManager(), SLOT(clearWaypointList()));
        qDebug() << "Requesting waypoints";
        emit requestWaypoints();
    }
}

void WaypointList::setWaypoint(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool current)
{
    if (uasId == this->uas->getUASID())
    {
        transmitDelay->start(1000);
        QString string = "New waypoint";

        if (waypointNames.contains(id))
        {
            string = waypointNames.value(id);
        }

        Waypoint* wp = new Waypoint(string, id, x, y, z, yaw, autocontinue, current);
        addWaypoint(wp);
    }
}

void WaypointList::waypointReached(UASInterface* uas, int waypointId)
{
    Q_UNUSED(uas);
    if (waypoints.size() > waypointId)
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
    }
}

void WaypointList::transmit()
{
    transmitDelay->start(1000);
    m_ui->transmitButton->setEnabled(false);
    emit clearWaypointList();
    waypointNames.clear();
    for(int i = 0; i < waypoints.size(); i++)
    {
        Waypoint* wp = waypoints[i];
        waypointNames.insert(wp->id, wp->name);
        emit waypointChanged(wp);
        if (wp->current)
            emit currentWaypointChanged(wp->id);
    }
    while(waypoints.size()>0)
    {
        removeWaypoint(waypoints[0]);
    }
    emit requestWaypoints();
}

void WaypointList::add()
{
    // Only add waypoints if UAS is present
    if (uas)
    {
        if (waypoints.size() > 0)
        {
            addWaypoint(new Waypoint("New waypoint", waypoints.size(), 0.0, 0.1, -0.5, 0.0, false, false));
        }
        else
        {
            addWaypoint(new Waypoint("New waypoint", waypoints.size(), 0.0, 0.0, -0.5, 360.0, false, true));
        }
    }
}

void WaypointList::addWaypoint(Waypoint* wp)
{
    if (waypoints.contains(wp))
    {
        removeWaypoint(wp);
    }

    waypoints.insert(wp->id, wp);

    if (!wpViews.contains(wp))
    {
        WaypointView* wpview = new WaypointView(wp, this);
        wpViews.insert(wp, wpview);
        listLayout->addWidget(wpViews.value(wp));
        connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)), this, SLOT(moveDown(Waypoint*)));
        connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)), this, SLOT(moveUp(Waypoint*)));
        connect(wpview, SIGNAL(removeWaypoint(Waypoint*)), this, SLOT(removeWaypointAndName(Waypoint*)));
        connect(wpview, SIGNAL(setCurrentWaypoint(Waypoint*)), this, SLOT(setCurrentWaypoint(Waypoint*)));
        connect(wpview, SIGNAL(waypointUpdated(Waypoint*)), this, SIGNAL(waypointChanged(Waypoint*)));
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

void WaypointList::debugOutputWaypoints()
{
    for (int i = 0; i < waypoints.size(); i++)
    {
        qDebug() << i << " " << waypoints[i]->name;
    }
}

void WaypointList::moveUp(Waypoint* wp)
{
    int id = wp->id;
    if (waypoints.size() > 1 && waypoints.size() > id)
    {
        Waypoint* temp = waypoints[id];
        if (id > 0)
        {
            waypoints[id] = waypoints[id-1];
            waypoints[id-1] = temp;
            waypoints[id-1]->id = id-1;
            waypoints[id]->id = id;
        }
        else
        {
            waypoints[id] = waypoints[waypoints.size()-1];
            waypoints[waypoints.size()-1] = temp;
            waypoints[waypoints.size()-1]->id = waypoints.size()-1;
            waypoints[id]->id = id;
        }
        redrawList();
    }
}

void WaypointList::moveDown(Waypoint* wp)
{
    int id = wp->id;
    if (waypoints.size() > 1 && waypoints.size() > id)
    {
        Waypoint* temp = waypoints[id];
        if (id != waypoints.size()-1)
        {
            waypoints[id] = waypoints[id+1];
            waypoints[id+1] = temp;
            waypoints[id+1]->id = id+1;
            waypoints[id]->id = id;
        }
        else
        {
            waypoints[id] = waypoints[0];
            waypoints[0] = temp;
            waypoints[0]->id = 0;
            waypoints[id]->id = id;
        }
        redrawList();
    }
}

void WaypointList::removeWaypointAndName(Waypoint* wp)
{
    waypointNames.remove(wp->id);
    removeWaypoint(wp);
}

void WaypointList::removeWaypoint(Waypoint* wp)
{
    // Delete from list
    if (wp != NULL){
        waypoints.remove(wp->id);
        for(int i = wp->id; i < waypoints.size(); i++)
        {
            waypoints[i]->id = i;
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
            waypoints[i]->current = true;
            // Retransmit waypoint
            //uas->getWaypointManager().setWaypointActive(i);
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
        in << wp->name << "~" << wp->id << "~" << wp->x << "~" << wp->y << "~" << wp->z << "~" << wp->yaw << "~" << wp->autocontinue << "~" << wp->current << "\n";
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
        removeWaypointAndName(waypoints[0]);
    }

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QStringList wpParams = in.readLine().split("~");
        if (wpParams.size() == 8)
            addWaypoint(new Waypoint(wpParams[0], wpParams[1].toInt(), wpParams[2].toDouble(), wpParams[3].toDouble(), wpParams[4].toDouble(), wpParams[5].toDouble(), (wpParams[6].toInt() == 1 ? true : false), (wpParams[7].toInt() == 1 ? true : false)));
    }
    file.close();
}


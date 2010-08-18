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
        mavX(0.0),
        mavY(0.0),
        mavZ(0.0),
        mavYaw(0.0),
        m_ui(new Ui::WaypointList)
{
    m_ui->setupUi(this);

    listLayout = new QVBoxLayout(m_ui->listWidget);
    listLayout->setSpacing(6);
    listLayout->setMargin(0);
    listLayout->setAlignment(Qt::AlignTop);
    m_ui->listWidget->setLayout(listLayout);

    this->uas = NULL;

    // ADD WAYPOINT
    // Connect add action, set right button icon and connect action to this class
    connect(m_ui->addButton, SIGNAL(clicked()), m_ui->actionAddWaypoint, SIGNAL(triggered()));
    connect(m_ui->actionAddWaypoint, SIGNAL(triggered()), this, SLOT(add()));

    // ADD WAYPOINT AT CURRENT POSITION
    connect(m_ui->positionAddButton, SIGNAL(clicked()), this, SLOT(addCurrentPositonWaypoint()));

    // SEND WAYPOINTS
    connect(m_ui->transmitButton, SIGNAL(clicked()), this, SLOT(transmit()));

    // REQUEST WAYPOINTS
    connect(m_ui->readButton, SIGNAL(clicked()), this, SLOT(read()));

    // SAVE/LOAD WAYPOINTS
    connect(m_ui->saveButton, SIGNAL(clicked()), this, SLOT(saveWaypoints()));
    connect(m_ui->loadButton, SIGNAL(clicked()), this, SLOT(loadWaypoints()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));

    // SET UAS AFTER ALL SIGNALS/SLOTS ARE CONNECTED
    setUAS(uas);

    // STATUS LABEL
    updateStatusLabel("");

    this->setVisible(false);
}

WaypointList::~WaypointList()
{
    delete m_ui;
}

void WaypointList::updateLocalPosition(UASInterface* uas, double x, double y, double z, quint64 usec)
{
    Q_UNUSED(uas);
    Q_UNUSED(usec);
    mavX = x;
    mavY = y;
    mavZ = z;
}

void WaypointList::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 usec)
{
    Q_UNUSED(uas);
    Q_UNUSED(usec);
    Q_UNUSED(roll);
    Q_UNUSED(pitch);
    mavYaw = yaw;
}

void WaypointList::setUAS(UASInterface* uas)
{
    if (this->uas == NULL && uas != NULL)
    {
        this->uas = uas;

        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),  this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),       this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));

        connect(&uas->getWaypointManager(), SIGNAL(updateStatusString(const QString &)),        this, SLOT(updateStatusLabel(const QString &)));
        connect(&uas->getWaypointManager(), SIGNAL(waypointListChanged(void)),                  this, SLOT(waypointListChanged(void)));
        connect(&uas->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)),            this, SLOT(currentWaypointChanged(quint16)));
    }
}

void WaypointList::saveWaypoints()
{
    if (uas)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "./waypoints.txt", tr("Waypoint File (*.txt)"));
        uas->getWaypointManager().localSaveWaypoints(fileName);
    }
}

void WaypointList::loadWaypoints()
{
    if (uas)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Waypoint File (*.txt)"));
        uas->getWaypointManager().localLoadWaypoints(fileName);
    }
}

void WaypointList::transmit()
{
    if (uas)
    {
        uas->getWaypointManager().writeWaypoints();
    }
}

void WaypointList::read()
{
    if (uas)
    {
        uas->getWaypointManager().readWaypoints();
    }
}

void WaypointList::add()
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager().getWaypointList();
        if (waypoints.size() > 0)
        {
            Waypoint *last = waypoints.at(waypoints.size()-1);
            Waypoint *wp = new Waypoint(0, last->getX(), last->getY(), last->getZ(), last->getYaw(), last->getAutoContinue(), false, last->getOrbit(), last->getHoldTime());
            uas->getWaypointManager().localAddWaypoint(wp);
        }
        else
        {
            Waypoint *wp = new Waypoint(0, 1.1, 1.1, -0.8, 0.0, true, true, 0.15, 2000);
            uas->getWaypointManager().localAddWaypoint(wp);
        }
    }
}

void WaypointList::addCurrentPositonWaypoint()
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager().getWaypointList();
        if (waypoints.size() > 0)
        {
            Waypoint *last = waypoints.at(waypoints.size()-1);
            Waypoint *wp = new Waypoint(0, (qRound(mavX*100))/100., (qRound(mavY*100))/100., (qRound(mavZ*100))/100., (qRound(mavYaw*100))/100., last->getAutoContinue(), false, last->getOrbit(), last->getHoldTime());
            uas->getWaypointManager().localAddWaypoint(wp);
        }
        else
        {
            Waypoint *wp = new Waypoint(0, (qRound(mavX*100))/100., (qRound(mavY*100))/100., (qRound(mavZ*100))/100., (qRound(mavYaw*100))/100., true, true, 0.15, 2000);
            uas->getWaypointManager().localAddWaypoint(wp);
        }
    }
}

void WaypointList::updateStatusLabel(const QString &string)
{
    m_ui->statusLabel->setText(string);
}

void WaypointList::changeCurrentWaypoint(quint16 seq)
{
    if (uas)
    {
        uas->getWaypointManager().setCurrentWaypoint(seq);
    }
}

void WaypointList::currentWaypointChanged(quint16 seq)
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager().getWaypointList();

        if (seq < waypoints.size())
        {
            for(int i = 0; i < waypoints.size(); i++)
            {
                WaypointView* widget = wpViews.find(waypoints[i]).value();

                if (waypoints[i]->getId() == seq)
                {
                    widget->setCurrent(true);
                }
                else
                {
                    widget->setCurrent(false);
                }
            }
        }
    }
}

void WaypointList::waypointListChanged()
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager().getWaypointList();

        // first remove all views of non existing waypoints
        if (!wpViews.empty())
        {
            QMapIterator<Waypoint*,WaypointView*> viewIt(wpViews);
            viewIt.toFront();
            while(viewIt.hasNext())
            {
                viewIt.next();
                Waypoint *cur = viewIt.key();
                int i;
                for (i = 0; i < waypoints.size(); i++)
                {
                    if (waypoints[i] == cur)
                    {
                        break;
                    }
                }
                if (i == waypoints.size())
                {
                    WaypointView* widget = wpViews.find(cur).value();
                    widget->hide();
                    listLayout->removeWidget(widget);
                    wpViews.remove(cur);
                }
            }
        }

        // then add/update the views for each waypoint in the list
        for(int i = 0; i < waypoints.size(); i++)
        {
            Waypoint *wp = waypoints[i];
            if (!wpViews.contains(wp))
            {
                WaypointView* wpview = new WaypointView(wp, this);
                wpViews.insert(wp, wpview);
                connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)),    this, SLOT(moveDown(Waypoint*)));
                connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)),      this, SLOT(moveUp(Waypoint*)));
                connect(wpview, SIGNAL(removeWaypoint(Waypoint*)),      this, SLOT(removeWaypoint(Waypoint*)));
                connect(wpview, SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointChanged(quint16)));
                connect(wpview, SIGNAL(changeCurrentWaypoint(quint16)), this, SLOT(changeCurrentWaypoint(quint16)));
            }

            WaypointView *wpv = wpViews.value(wp);
            wpv->updateValues();    // update the values of the ui elements in the view
            listLayout->addWidget(wpv);
        }
    }
}

void WaypointList::moveUp(Waypoint* wp)
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager().getWaypointList();

        //get the current position of wp in the local storage
        int i;
        for (i = 0; i < waypoints.size(); i++)
        {
            if (waypoints[i] == wp)
                break;
        }

        // if wp was found and its not the first entry, move it
        if (i < waypoints.size() && i > 0)
        {
            uas->getWaypointManager().localMoveWaypoint(i, i-1);
        }
    }
}

void WaypointList::moveDown(Waypoint* wp)
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager().getWaypointList();

        //get the current position of wp in the local storage
        int i;
        for (i = 0; i < waypoints.size(); i++)
        {
            if (waypoints[i] == wp)
                break;
        }

        // if wp was found and its not the last entry, move it
        if (i < waypoints.size()-1)
        {
            uas->getWaypointManager().localMoveWaypoint(i, i+1);
        }
    }
}

void WaypointList::removeWaypoint(Waypoint* wp)
{
    if (uas)
    {
        uas->getWaypointManager().localRemoveWaypoint(wp->getId());
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


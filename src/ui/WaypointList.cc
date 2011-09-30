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
#include <QMessageBox>
#include <QMouseEvent>

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

    //EDIT TAB

    editableListLayout = new QVBoxLayout(m_ui->editableListWidget);
    editableListLayout->setSpacing(0);
    editableListLayout->setMargin(0);
    editableListLayout->setAlignment(Qt::AlignTop);
    m_ui->editableListWidget->setLayout(editableListLayout);

    // ADD WAYPOINT
    // Connect add action, set right button icon and connect action to this class
    connect(m_ui->addButton, SIGNAL(clicked()), m_ui->actionAddWaypoint, SIGNAL(triggered()));
    connect(m_ui->actionAddWaypoint, SIGNAL(triggered()), this, SLOT(addEditable()));

    // ADD WAYPOINT AT CURRENT POSITION
    connect(m_ui->positionAddButton, SIGNAL(clicked()), this, SLOT(addCurrentPositionWaypoint()));

    // SEND WAYPOINTS
    connect(m_ui->transmitButton, SIGNAL(clicked()), this, SLOT(transmit()));

    // REQUEST WAYPOINTS
    connect(m_ui->readButton, SIGNAL(clicked()), this, SLOT(read()));

    // SAVE/LOAD WAYPOINTS
    connect(m_ui->saveButton, SIGNAL(clicked()), this, SLOT(saveWaypoints()));
    connect(m_ui->loadButton, SIGNAL(clicked()), this, SLOT(loadWaypoints()));

    //connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));

    //VIEW TAB

    viewOnlyListLayout = new QVBoxLayout(m_ui->viewOnlyListWidget);
    viewOnlyListLayout->setSpacing(0);
    viewOnlyListLayout->setMargin(0);
    viewOnlyListLayout->setAlignment(Qt::AlignTop);
    m_ui->viewOnlyListWidget->setLayout(viewOnlyListLayout);

    // REFRESH VIEW TAB

     connect(m_ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));


    // SET UAS AFTER ALL SIGNALS/SLOTS ARE CONNECTED
    setUAS(uas);

    // STATUS LABEL
    updateStatusLabel("");

    this->setVisible(false);
    loadFileGlobalWP = false;
    readGlobalWP = false;
    centerMapCoordinate.setX(0.0);
    centerMapCoordinate.setY(0.0);

}

WaypointList::~WaypointList()
{
    delete m_ui;
}

void WaypointList::updatePosition(UASInterface* uas, double x, double y, double z, quint64 usec)
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

        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),  this, SLOT(updatePosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),       this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));

        connect(uas->getWaypointManager(), SIGNAL(updateStatusString(const QString &)),        this, SLOT(updateStatusLabel(const QString &)));
        connect(uas->getWaypointManager(), SIGNAL(waypointEditableListChanged(void)),                  this, SLOT(waypointEditableListChanged(void)));
        connect(uas->getWaypointManager(), SIGNAL(waypointEditableChanged(int,Waypoint*)), this, SLOT(updateWaypointEditable(int,Waypoint*)));
        connect(uas->getWaypointManager(), SIGNAL(waypointViewOnlyListChanged(void)),                  this, SLOT(waypointViewOnlyListChanged(void)));
        connect(uas->getWaypointManager(), SIGNAL(waypointViewOnlyChanged(int,Waypoint*)), this, SLOT(updateWaypointViewOnly(int,Waypoint*)));
        connect(uas->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)),            this, SLOT(currentWaypointViewOnlyChanged(quint16)));
        //connect(uas->getWaypointManager(),SIGNAL(loadWPFile()),this,SLOT(setIsLoadFileWP()));
        //connect(uas->getWaypointManager(),SIGNAL(readGlobalWPFromUAS(bool)),this,SLOT(setIsReadGlobalWP(bool)));
    }
}

void WaypointList::saveWaypoints()
{
    if (uas)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "./waypoints.txt", tr("Waypoint File (*.txt)"));
        uas->getWaypointManager()->saveWaypoints(fileName);
    }
}

void WaypointList::loadWaypoints()
{
    if (uas)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), ".", tr("Waypoint File (*.txt)"));
        uas->getWaypointManager()->loadWaypoints(fileName);
    }
}

void WaypointList::transmit()
{
    if (uas)
    {
        uas->getWaypointManager()->writeWaypoints();
    }
}

void WaypointList::read()
{
    if (uas)
    {
        uas->getWaypointManager()->readWaypoints(true);
    }
}

void WaypointList::refresh()
{
    if (uas)
    {
        uas->getWaypointManager()->readWaypoints(false);
    }
}

void WaypointList::addEditable()
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();
        Waypoint *wp;
        if (waypoints.size() > 0)
        {
            // Create waypoint with last frame
            Waypoint *last = waypoints.at(waypoints.size()-1);
            wp = new Waypoint(0, last->getX(), last->getY(), last->getZ(), last->getParam1(), last->getParam2(), last->getParam3(), last->getParam4(),
                              last->getAutoContinue(), false, last->getFrame(), last->getAction());
            uas->getWaypointManager()->addWaypointEditable(wp);
        }
        else
        {
            // Create first waypoint at current MAV position
            addCurrentPositionWaypoint();
        }
    }
}

/*
void WaypointList::addViewOnly()
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointViewOnlyList();
        Waypoint *wp;
        if (waypoints.size() > 0)
        {
            // Create waypoint with last frame
            Waypoint *last = waypoints.at(waypoints.size()-1);
            wp = new Waypoint(0, last->getX(), last->getY(), last->getZ(), last->getParam1(), last->getParam2(), last->getParam3(), last->getParam4(),
                              last->getAutoContinue(), false, last->getFrame(), last->getAction());
            uas->getWaypointManager()->addWaypointEditable(wp);
        }
        else
        {
            // Create first waypoint at current MAV position
            addCurrentPositionWaypoint();
        }
    }
}
*/

void WaypointList::addCurrentPositionWaypoint()
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();
        Waypoint *wp;
        Waypoint *last = 0;
        if (waypoints.size() > 0)
        {
            last = waypoints.at(waypoints.size()-1);
        }

        if (uas->globalPositionKnown())
        {
            float acceptanceRadiusGlobal = 10.0f;
            float holdTime = 0.0f;
            float yawGlobal = 0.0f;
            if (last)
            {
                acceptanceRadiusGlobal = last->getAcceptanceRadius();
                holdTime = last->getHoldTime();
                yawGlobal = last->getYaw();
            }
            // Create global frame waypoint per default
            wp = new Waypoint(0, uas->getLatitude(), uas->getLongitude(), uas->getAltitude(), 0, acceptanceRadiusGlobal, holdTime, yawGlobal, true, true, MAV_FRAME_GLOBAL_RELATIVE_ALT, MAV_CMD_NAV_WAYPOINT);
            uas->getWaypointManager()->addWaypointEditable(wp);
            updateStatusLabel(tr("Added GLOBAL, ALTITUDE OVER GROUND waypoint"));
        }
        else if (uas->localPositionKnown())
        {
            float acceptanceRadiusLocal = 0.2f;
            float holdTime = 0.5f;
            if (last)
            {
                acceptanceRadiusLocal = last->getAcceptanceRadius();
                holdTime = last->getHoldTime();
            }
            // Create local frame waypoint as second option
            wp = new Waypoint(0, uas->getLocalX(), uas->getLocalY(), uas->getLocalZ(), uas->getYaw(), acceptanceRadiusLocal, holdTime, 0.0, true, true, MAV_FRAME_LOCAL_NED, MAV_CMD_NAV_WAYPOINT);
            uas->getWaypointManager()->addWaypointEditable(wp);
            updateStatusLabel(tr("Added LOCAL (NED) waypoint"));
        }
        else
        {
            // Do nothing
            updateStatusLabel(tr("Not adding waypoint, no position known of MAV known yet."));
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
        uas->getWaypointManager()->setCurrentWaypoint(seq);
    }
}


void WaypointList::currentWaypointEditableChanged(quint16 seq)
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();

        if (seq < waypoints.size())
        {
            for(int i = 0; i < waypoints.size(); i++)
            {
                WaypointEditableView* widget = wpEditableViews.find(waypoints[i]).value();

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

void WaypointList::currentWaypointViewOnlyChanged(quint16 seq)
{
    if (uas)
    {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointViewOnlyList();

        if (seq < waypoints.size())
        {
            for(int i = 0; i < waypoints.size(); i++)
            {
                WaypointViewOnlyView* widget = wpViewOnlyViews.find(waypoints[i]).value();

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

void WaypointList::updateWaypointEditable(int uas, Waypoint* wp)
{
    Q_UNUSED(uas);
    WaypointEditableView *wpv = wpEditableViews.value(wp);
    wpv->updateValues();
}

void WaypointList::updateWaypointViewOnly(int uas, Waypoint* wp)
{
    Q_UNUSED(uas);
    WaypointViewOnlyView *wpv = wpViewOnlyViews.value(wp);
    wpv->updateValues();
}

void WaypointList::waypointViewOnlyListChanged()
{
    if (uas) {
        // Prevent updates to prevent visual flicker
        this->setUpdatesEnabled(false);
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointViewOnlyList();

        if (!wpViewOnlyViews.empty()) {
            QMapIterator<Waypoint*,WaypointViewOnlyView*> viewIt(wpViewOnlyViews);
            viewIt.toFront();
            while(viewIt.hasNext()) {
                viewIt.next();
                Waypoint *cur = viewIt.key();
                int i;
                for (i = 0; i < waypoints.size(); i++) {
                    if (waypoints[i] == cur) {
                        break;
                    }
                }
                if (i == waypoints.size()) {
                    WaypointViewOnlyView* widget = wpViewOnlyViews.find(cur).value();
                    widget->hide();
                    viewOnlyListLayout->removeWidget(widget);
                    wpViewOnlyViews.remove(cur);
                }
            }
        }

        // then add/update the views for each waypoint in the list
        for(int i = 0; i < waypoints.size(); i++) {
            Waypoint *wp = waypoints[i];
            if (!wpViewOnlyViews.contains(wp)) {
                WaypointViewOnlyView* wpview = new WaypointViewOnlyView(wp, this);
                wpViewOnlyViews.insert(wp, wpview);
                connect(wpview, SIGNAL(changeCurrentWaypoint(quint16)), this, SLOT(changeCurrentWaypoint(quint16)));
                viewOnlyListLayout->insertWidget(i, wpview);
            }
            WaypointViewOnlyView *wpv = wpViewOnlyViews.value(wp);

            //check if ordering has changed
            if(viewOnlyListLayout->itemAt(i)->widget() != wpv) {
                viewOnlyListLayout->removeWidget(wpv);
                viewOnlyListLayout->insertWidget(i, wpv);
            }

            wpv->updateValues();    // update the values of the ui elements in the view
        }
        this->setUpdatesEnabled(true);
        loadFileGlobalWP = false;
    }
}


void WaypointList::waypointEditableListChanged()
{
    if (uas) {
        // Prevent updates to prevent visual flicker
        this->setUpdatesEnabled(false);
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();

        if (!wpEditableViews.empty()) {
            QMapIterator<Waypoint*,WaypointEditableView*> viewIt(wpEditableViews);
            viewIt.toFront();
            while(viewIt.hasNext()) {
                viewIt.next();
                Waypoint *cur = viewIt.key();
                int i;
                for (i = 0; i < waypoints.size(); i++) {
                    if (waypoints[i] == cur) {
                        break;
                    }
                }
                if (i == waypoints.size()) {
                    WaypointEditableView* widget = wpEditableViews.find(cur).value();
                    widget->hide();
                    editableListLayout->removeWidget(widget);
                    wpEditableViews.remove(cur);
                }
            }
        }

        // then add/update the views for each waypoint in the list
        for(int i = 0; i < waypoints.size(); i++) {
            Waypoint *wp = waypoints[i];
            if (!wpEditableViews.contains(wp)) {
                WaypointEditableView* wpview = new WaypointEditableView(wp, this);
                wpEditableViews.insert(wp, wpview);
                connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)),    this, SLOT(moveDown(Waypoint*)));
                connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)),      this, SLOT(moveUp(Waypoint*)));
                connect(wpview, SIGNAL(removeWaypoint(Waypoint*)),      this, SLOT(removeWaypoint(Waypoint*)));
                //connect(wpview, SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointChanged(quint16))); commented, because unused
                connect(wpview, SIGNAL(changeCurrentWaypoint(quint16)), this, SLOT(changeCurrentWaypointEditable(quint16)));
                editableListLayout->insertWidget(i, wpview);
            }
            WaypointEditableView *wpv = wpEditableViews.value(wp);

            //check if ordering has changed
            if(editableListLayout->itemAt(i)->widget() != wpv) {
                editableListLayout->removeWidget(wpv);
                editableListLayout->insertWidget(i, wpv);
            }

            wpv->updateValues();    // update the values of the ui elements in the view
        }
        this->setUpdatesEnabled(true);
        loadFileGlobalWP = false;
    }
}

//void WaypointList::waypointEditableListChanged()
//{
//    if (uas)
//    {
//        // Prevent updates to prevent visual flicker
//        this->setUpdatesEnabled(false);
//        // Get all waypoints
//        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();

////        // Store the current state, then check which widgets to update
////        // and which ones to delete
////        QList<Waypoint*> oldWaypoints = wpEditableViews.keys();

////        foreach (Waypoint* wp, waypoints)
////        {
////            WaypointEditableView* wpview;
////            // Create any new waypoint
////            if (!wpEditableViews.contains(wp))
////            {
////                wpview = new WaypointEditableView(wp, this);
////                wpEditableViews.insert(wp, wpview);
////                connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)),    this, SLOT(moveDown(Waypoint*)));
////                connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)),      this, SLOT(moveUp(Waypoint*)));
////                connect(wpview, SIGNAL(removeWaypoint(Waypoint*)),      this, SLOT(removeWaypoint(Waypoint*)));
////                connect(wpview, SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointChanged(quint16)));
////                connect(wpview, SIGNAL(changeCurrentWaypoint(quint16)), this, SLOT(changeCurrentWaypoint(quint16)));
////                editableListLayout->addWidget(wpview);
////            }
////            else
////            {
////                // Update existing waypoints
////                wpview = wpEditableViews.value(wp);

////            }
////            // Mark as updated by removing from old list
////            oldWaypoints.removeAt(oldWaypoints.indexOf(wp));

////            wpview->updateValues();    // update the values of the ui elements in the view

////        }

////        // The old list now contains all wps to be deleted
////        foreach (Waypoint* wp, oldWaypoints)
////        {
////            // Delete waypoint view and entry in list
////            WaypointEditableView* wpv = wpEditableViews.value(wp);
////            if (wpv)
////            {
////                editableListLayout->removeWidget(wpv);
////                delete wpv;
////            }
////            wpEditableViews.remove(wp);
////        }

//        if (!wpEditableViews.empty())
//        {
//            QMapIterator<Waypoint*,WaypointEditableView*> viewIt(wpEditableViews);
//            viewIt.toFront();
//            while(viewIt.hasNext())
//            {
//                viewIt.next();
//                Waypoint *cur = viewIt.key();
//                int i;
//                for (i = 0; i < waypoints.size(); i++)
//                {
//                    if (waypoints[i] == cur)
//                    {
//                        break;
//                    }
//                }
//                if (i == waypoints.size())
//                {
//                    WaypointEditableView* widget = wpEditableViews.find(cur).value();
//                    if (widget)
//                    {
//                        widget->hide();
//                        editableListLayout->removeWidget(widget);
//                    }
//                    wpEditableViews.remove(cur);
//                }
//            }
//        }

//        // then add/update the views for each waypoint in the list
//        for(int i = 0; i < waypoints.size(); i++)
//        {
//            Waypoint *wp = waypoints[i];
//            if (!wpEditableViews.contains(wp))
//            {
//                WaypointEditableView* wpview = new WaypointEditableView(wp, this);
//                wpEditableViews.insert(wp, wpview);
//                connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)),    this, SLOT(moveDown(Waypoint*)));
//                connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)),      this, SLOT(moveUp(Waypoint*)));
//                connect(wpview, SIGNAL(removeWaypoint(Waypoint*)),      this, SLOT(removeWaypoint(Waypoint*)));
//                connect(wpview, SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointChanged(quint16)));
//                connect(wpview, SIGNAL(changeCurrentWaypoint(quint16)), this, SLOT(changeCurrentWaypoint(quint16)));
//            }
//            WaypointEditableView *wpv = wpEditableViews.value(wp);
//            wpv->updateValues();    // update the values of the ui elements in the view
//            editableListLayout->addWidget(wpv);

//        }
//        this->setUpdatesEnabled(true);
//    }
////    loadFileGlobalWP = false;
//}

void WaypointList::moveUp(Waypoint* wp)
{
    if (uas) {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();

        //get the current position of wp in the local storage
        int i;
        for (i = 0; i < waypoints.size(); i++) {
            if (waypoints[i] == wp)
                break;
        }

        // if wp was found and its not the first entry, move it
        if (i < waypoints.size() && i > 0) {
            uas->getWaypointManager()->moveWaypoint(i, i-1);
        }
    }
}

void WaypointList::moveDown(Waypoint* wp)
{
    if (uas) {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();

        //get the current position of wp in the local storage
        int i;
        for (i = 0; i < waypoints.size(); i++) {
            if (waypoints[i] == wp)
                break;
        }

        // if wp was found and its not the last entry, move it
        if (i < waypoints.size()-1) {
            uas->getWaypointManager()->moveWaypoint(i, i+1);
        }
    }
}

void WaypointList::removeWaypoint(Waypoint* wp)
{
    if (uas) {
        uas->getWaypointManager()->removeWaypoint(wp->getId());
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



void WaypointList::on_clearWPListButton_clicked()
{


    if (uas) {
        emit clearPathclicked();
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();
        while(!waypoints.isEmpty()) { //for(int i = 0; i <= waypoints.size(); i++)
            WaypointEditableView* widget = wpEditableViews.find(waypoints[0]).value();
            widget->remove();
        }
    } else {
//        if(isGlobalWP)
//        {
//           emit clearPathclicked();
//        }
    }
}

///** @brief The MapWidget informs that a waypoint global was changed on the map */

//void WaypointList::waypointGlobalChanged(QPointF coordinate, int indexWP)
//{
//    if (uas)
//    {
//        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();
//        if (waypoints.size() > 0)
//        {
//            Waypoint *temp = waypoints.at(indexWP);

//            temp->setX(coordinate.x());
//            temp->setY(coordinate.y());

//            //WaypointGlobalView* widget = wpGlobalViews.find(waypoints[indexWP]).value();
//            //widget->updateValues();
//        }
//    }


//}

///** @brief The MapWidget informs that a waypoint global was changed on the map */

//void WaypointList::waypointGlobalPositionChanged(Waypoint* wp)
//{
//    QPointF coordinate;
//    coordinate.setX(wp->getX());
//    coordinate.setY(wp->getY());

//   emit ChangeWaypointGlobalPosition(wp->getId(), coordinate);


//}

void WaypointList::clearWPWidget()
{
    if (uas) {
        const QVector<Waypoint *> &waypoints = uas->getWaypointManager()->getWaypointEditableList();
        while(!waypoints.isEmpty()) { //for(int i = 0; i <= waypoints.size(); i++)
            WaypointEditableView* widget = wpEditableViews.find(waypoints[0]).value();
            widget->remove();
        }
    }
}

//void WaypointList::setIsLoadFileWP()
//{
//    loadFileGlobalWP = true;
//}

//void WaypointList::setIsReadGlobalWP(bool value)
//{
//    // FIXME James Check this
//    Q_UNUSED(value);
//    // readGlobalWP = value;
//}

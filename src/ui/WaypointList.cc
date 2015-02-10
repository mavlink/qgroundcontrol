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
#include "QGCFileDialog.h"

#include <UASInterface.h>
#include <UAS.h>
#include <UASManager.h>
#include <QDebug>
#include <QMouseEvent>
#include <QTextEdit>

WaypointList::WaypointList(QWidget *parent, UASWaypointManager* wpm) :
    QWidget(parent),
    uas(NULL),
    WPM(wpm),
    mavX(0.0),
    mavY(0.0),
    mavZ(0.0),
    mavYaw(0.0),
    showOfflineWarning(false),
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

    // DELETE ALL WAYPOINTS
    connect(m_ui->clearWPListButton, SIGNAL(clicked()), this, SLOT(clearWPWidget()));

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

    if (WPM) {
        // SET UAS AFTER ALL SIGNALS/SLOTS ARE CONNECTED
        if (!WPM->getUAS())
        {
            // Disable buttons which don't make sense without valid UAS.
            m_ui->positionAddButton->setEnabled(false);
            m_ui->transmitButton->setEnabled(false);
            m_ui->readButton->setEnabled(false);
            m_ui->refreshButton->setEnabled(false);

            //FIXME: The whole "Onboard Waypoints"-tab should be hidden, instead of "refresh" button
            // Insert a "NO UAV" info into the Onboard Tab
            QLabel* noUas = new QLabel(this);
            noUas->setObjectName("noUas");
            noUas->setText("NO UAS");
            noUas->setEnabled(false);
            noUas->setAlignment(Qt::AlignCenter);
            viewOnlyListLayout->insertWidget(0, noUas);

            showOfflineWarning = true;
        } else {
            setUAS(static_cast<UASInterface*>(WPM->getUAS()));
        }

        /* connect slots */
        connect(WPM, SIGNAL(updateStatusString(const QString &)),        this, SLOT(updateStatusLabel(const QString &)));
        connect(WPM, SIGNAL(waypointEditableListChanged(void)),                  this, SLOT(waypointEditableListChanged(void)));
        connect(WPM, SIGNAL(waypointEditableChanged(int,Waypoint*)), this, SLOT(updateWaypointEditable(int,Waypoint*)));
        connect(WPM, SIGNAL(waypointViewOnlyListChanged(void)),                  this, SLOT(waypointViewOnlyListChanged(void)));
        connect(WPM, SIGNAL(waypointViewOnlyChanged(int,Waypoint*)), this, SLOT(updateWaypointViewOnly(int,Waypoint*)));
        connect(WPM, SIGNAL(currentWaypointChanged(quint16)),            this, SLOT(currentWaypointViewOnlyChanged(quint16)));

        //Even if there are no waypoints, since this is a new instance and there is an
        //existing WPM, then we need to assume things have changed, and act appropriatly.
        waypointEditableListChanged();
        waypointViewOnlyListChanged();
    }

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
    if (!uas)
        return;

    if (this->uas != NULL)
    {
        // Clear current list
        on_clearWPListButton_clicked();
        // Disconnect everything
        disconnect(WPM, SIGNAL(updateStatusString(const QString &)),        this, SLOT(updateStatusLabel(const QString &)));
        disconnect(WPM, SIGNAL(waypointEditableListChanged(void)),                  this, SLOT(waypointEditableListChanged(void)));
        disconnect(WPM, SIGNAL(waypointEditableChanged(int,Waypoint*)), this, SLOT(updateWaypointEditable(int,Waypoint*)));
        disconnect(WPM, SIGNAL(waypointViewOnlyListChanged(void)),                  this, SLOT(waypointViewOnlyListChanged(void)));
        disconnect(WPM, SIGNAL(waypointViewOnlyChanged(int,Waypoint*)), this, SLOT(updateWaypointViewOnly(int,Waypoint*)));
        disconnect(WPM, SIGNAL(currentWaypointChanged(quint16)),            this, SLOT(currentWaypointViewOnlyChanged(quint16)));
        disconnect(this->uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),  this, SLOT(updatePosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),       this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));
    }

    // Now that there's a valid UAS, enable the UI.
    m_ui->positionAddButton->setEnabled(true);
    m_ui->transmitButton->setEnabled(true);
    m_ui->readButton->setEnabled(true);
    m_ui->refreshButton->setEnabled(true);

    WPM = uas->getWaypointManager();

    this->uas = uas;
    connect(WPM, SIGNAL(updateStatusString(const QString &)),        this, SLOT(updateStatusLabel(const QString &)));
    connect(WPM, SIGNAL(waypointEditableListChanged(void)),                  this, SLOT(waypointEditableListChanged(void)));
    connect(WPM, SIGNAL(waypointEditableChanged(int,Waypoint*)), this, SLOT(updateWaypointEditable(int,Waypoint*)));
    connect(WPM, SIGNAL(waypointViewOnlyListChanged(void)),                  this, SLOT(waypointViewOnlyListChanged(void)));
    connect(WPM, SIGNAL(waypointViewOnlyChanged(int,Waypoint*)), this, SLOT(updateWaypointViewOnly(int,Waypoint*)));
    connect(WPM, SIGNAL(currentWaypointChanged(quint16)),            this, SLOT(currentWaypointViewOnlyChanged(quint16)));
    connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)),  this, SLOT(updatePosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)),       this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));
    //connect(WPM,SIGNAL(loadWPFile()),this,SLOT(setIsLoadFileWP()));
    //connect(WPM,SIGNAL(readGlobalWPFromUAS(bool)),this,SLOT(setIsReadGlobalWP(bool)));

    // Update list
    read();
}

void WaypointList::saveWaypoints()
{
    // TODO Need better default directory
    QString fileName = QGCFileDialog::getSaveFileName(this, tr("Save Waypoint File"), "./untitled.waypoints", tr("Waypoint Files (*.waypoints)"), "waypoints", true);
    WPM->saveWaypoints(fileName);
}

void WaypointList::loadWaypoints()
{
    // TODO Need better default directory
    QString fileName = QGCFileDialog::getOpenFileName(this, tr("Load Waypoint File"), ".", tr("Waypoint Files (*.waypoints);;All Files (*)"));
    WPM->loadWaypoints(fileName);
}

void WaypointList::transmit()
{
    if (uas)
    {
        WPM->writeWaypoints();
    }
}

void WaypointList::read()
{
    if (uas)
    {
        WPM->readWaypoints(true);
    }
}

void WaypointList::refresh()
{
    if (uas)
    {
        WPM->readWaypoints(false);
    }
}

void WaypointList::addEditable()
{
    addEditable(false);
}

void WaypointList::addEditable(bool onCurrentPosition)
{

    const QList<Waypoint *> &waypoints = WPM->getWaypointEditableList();
    Waypoint *wp;
    if (waypoints.count() > 0 &&
            !(onCurrentPosition && uas && (uas->localPositionKnown() || uas->globalPositionKnown())))
    {
        // Create waypoint with last frame
        Waypoint *last = waypoints.last();
        wp = WPM->createWaypoint();
//        wp->blockSignals(true);
        MAV_FRAME frame = (MAV_FRAME)last->getFrame();
        wp->setFrame(frame);
        if (frame == MAV_FRAME_GLOBAL || frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)
        {
            wp->setLatitude(last->getLatitude());
            wp->setLongitude(last->getLongitude());
            wp->setAltitude(last->getAltitude());
        } else {
            wp->setX(last->getX());
            wp->setY(last->getY());
            wp->setZ(last->getZ());
        }
        wp->setParam1(last->getParam1());
        wp->setParam2(last->getParam2());
        wp->setParam3(last->getParam3());
        wp->setParam4(last->getParam4());
        wp->setAutocontinue(last->getAutoContinue());
//        wp->blockSignals(false);
        wp->setAction(last->getAction());
//        WPM->addWaypointEditable(wp);
    }
    else
    {
        if (uas)
        {
            // Create first waypoint at current MAV position
            if (uas->globalPositionKnown())
            {
                if (onCurrentPosition)
                {

                    if (WPM->getFrameRecommendation() == MAV_FRAME_GLOBAL) {
                        updateStatusLabel(tr("Added default GLOBAL (Abs alt.) waypoint."));
                    } else {
                        updateStatusLabel(tr("Added default GLOBAL (Relative alt.) waypoint."));
                    }
                    wp = new Waypoint(0, uas->getLatitude(), uas->getLongitude(), uas->getAltitudeAMSL(), 0, WPM->getAcceptanceRadiusRecommendation(), 0, 0,true, true, (MAV_FRAME)WPM->getFrameRecommendation(), MAV_CMD_NAV_WAYPOINT);
                    WPM->addWaypointEditable(wp);

                } else {

                    if (WPM->getFrameRecommendation() == MAV_FRAME_GLOBAL) {
                        updateStatusLabel(tr("Added default GLOBAL (Abs alt.) waypoint."));
                    } else {
                        updateStatusLabel(tr("Added default GLOBAL (Relative alt.) waypoint."));
                    }
                    wp = new Waypoint(0, UASManager::instance()->getHomeLatitude(),
                                      UASManager::instance()->getHomeLongitude(),
                                      WPM->getAltitudeRecommendation(), 0, WPM->getAcceptanceRadiusRecommendation(), 0, 0,true, true, (MAV_FRAME)WPM->getFrameRecommendation(), MAV_CMD_NAV_WAYPOINT);
                    WPM->addWaypointEditable(wp);
                }
            }
            else if (uas->localPositionKnown())
            {
                if (onCurrentPosition)
                {
                    updateStatusLabel(tr("Added default LOCAL (NED) waypoint."));
                    wp = new Waypoint(0, uas->getLocalX(), uas->getLocalY(), uas->getLocalZ(), 0, WPM->getAcceptanceRadiusRecommendation(), 0, 0,true, true, MAV_FRAME_LOCAL_NED, MAV_CMD_NAV_WAYPOINT);
                    WPM->addWaypointEditable(wp);
                } else {
                    updateStatusLabel(tr("Added default LOCAL (NED) waypoint."));
                    wp = new Waypoint(0, 0, 0, -0.50, 0, WPM->getAcceptanceRadiusRecommendation(), 0, 0,true, true, MAV_FRAME_LOCAL_NED, MAV_CMD_NAV_WAYPOINT);
                    WPM->addWaypointEditable(wp);
                }
            }
            else
            {
                // MAV connected, but position unknown, add default waypoint
                updateStatusLabel(tr("WARNING: No position known. Adding default LOCAL (NED) waypoint"));
                wp = new Waypoint(0, UASManager::instance()->getHomeLatitude(),
                                  UASManager::instance()->getHomeLongitude(),
                                  WPM->getAltitudeRecommendation(), 0, WPM->getAcceptanceRadiusRecommendation(), 0, 0,true, true, (MAV_FRAME)WPM->getFrameRecommendation(), MAV_CMD_NAV_WAYPOINT);
                WPM->addWaypointEditable(wp);
            }
        }
        else
        {
            //Since no UAV available, create first default waypoint.
            updateStatusLabel(tr("No UAV connected. Adding default GLOBAL (NED) waypoint"));
            wp = new Waypoint(0, UASManager::instance()->getHomeLatitude(),
                              UASManager::instance()->getHomeLongitude(),
                              WPM->getAltitudeRecommendation(), 0, WPM->getAcceptanceRadiusRecommendation(), 0, 0,true, true, (MAV_FRAME)WPM->getFrameRecommendation(), MAV_CMD_NAV_WAYPOINT);
            WPM->addWaypointEditable(wp);
        }
    }
}

void WaypointList::addCurrentPositionWaypoint() {
    addEditable(true);
}

void WaypointList::updateStatusLabel(const QString &string)
{
    // Status label in write widget
    m_ui->statusLabel->setText(string);
    // Status label in read only widget
    m_ui->viewStatusLabel->setText(string);
}

// Request UASWaypointManager to send the SET_CURRENT message to UAV
void WaypointList::changeCurrentWaypoint(quint16 seq)
{
    if (uas)
    {
        WPM->setCurrentWaypoint(seq);
    }
}

// Request UASWaypointManager to set the new "current" and make sure all other waypoints are not "current"
void WaypointList::currentWaypointEditableChanged(quint16 seq)
{
        WPM->setCurrentEditable(seq);
        const QList<Waypoint *> waypoints = WPM->getWaypointEditableList();

        if (seq < waypoints.count())
        {
            for(int i = 0; i < waypoints.count(); i++)
            {
                WaypointEditableView* widget = wpEditableViews.value(waypoints[i], NULL);

                if (widget) {
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

// Update waypointViews to correctly indicate the new current waypoint
void WaypointList::currentWaypointViewOnlyChanged(quint16 seq)
{
    // First update the edit list
    currentWaypointEditableChanged(seq);

    const QList<Waypoint *> waypoints = WPM->getWaypointViewOnlyList();

    if (seq < waypoints.count())
    {
        for(int i = 0; i < waypoints.count(); i++)
        {
            WaypointViewOnlyView* widget = wpViewOnlyViews.value(waypoints[i], NULL);

            if (widget) {
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
    WaypointEditableView *wpv = wpEditableViews.value(wp, NULL);
    if (wpv) {
        wpv->updateValues();
    }
        m_ui->tabWidget->setCurrentIndex(0); // XXX magic number
}

void WaypointList::updateWaypointViewOnly(int uas, Waypoint* wp)
{
    Q_UNUSED(uas);
    WaypointViewOnlyView *wpv = wpViewOnlyViews.value(wp, NULL);
   if (wpv) {
       wpv->updateValues();
   }
   m_ui->tabWidget->setCurrentIndex(1); // XXX magic number
}

void WaypointList::waypointViewOnlyListChanged()
{
    // Prevent updates to prevent visual flicker
    this->setUpdatesEnabled(false);
    const QList<Waypoint *> &waypoints = WPM->getWaypointViewOnlyList();

    if (!wpViewOnlyViews.empty()) {
        QMapIterator<Waypoint*,WaypointViewOnlyView*> viewIt(wpViewOnlyViews);
        viewIt.toFront();
        while(viewIt.hasNext()) {
            viewIt.next();
            Waypoint *cur = viewIt.key();
            int i;
            for (i = 0; i < waypoints.count(); i++) {
                if (waypoints[i] == cur) {
                    break;
                }
            }
            if (i == waypoints.count()) {
                WaypointViewOnlyView* widget = wpViewOnlyViews.value(cur, NULL);
                if (widget) {
                    widget->hide();
                    viewOnlyListLayout->removeWidget(widget);
                    wpViewOnlyViews.remove(cur);
                }
            }
        }
    }

    // then add/update the views for each waypoint in the list
    for(int i = 0; i < waypoints.count(); i++) {
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

    m_ui->tabWidget->setCurrentIndex(1);

}


void WaypointList::waypointEditableListChanged()
{
    // Prevent updates to prevent visual flicker
    this->setUpdatesEnabled(false);
    const QList<Waypoint *> &waypoints = WPM->getWaypointEditableList();

    if (!wpEditableViews.empty()) {
        QMapIterator<Waypoint*,WaypointEditableView*> viewIt(wpEditableViews);
        viewIt.toFront();
        while(viewIt.hasNext()) {
            viewIt.next();
            Waypoint *cur = viewIt.key();
            int i;
            for (i = 0; i < waypoints.count(); i++) {
                if (waypoints[i] == cur) {
                    break;
                }
            }
            if (i == waypoints.count()) {
                WaypointEditableView* widget = wpEditableViews.value(cur, NULL);

                if (widget) {
                    widget->hide();
                    editableListLayout->removeWidget(widget);
                    wpEditableViews.remove(cur);
                }
            }
        }
    }

    // then add/update the views for each waypoint in the list
    for(int i = 0; i < waypoints.count(); i++) {
        Waypoint *wp = waypoints[i];
        if (!wpEditableViews.contains(wp)) {
            WaypointEditableView* wpview = new WaypointEditableView(wp, this);
            wpEditableViews.insert(wp, wpview);
            connect(wpview, SIGNAL(moveDownWaypoint(Waypoint*)),    this, SLOT(moveDown(Waypoint*)));
            connect(wpview, SIGNAL(moveUpWaypoint(Waypoint*)),      this, SLOT(moveUp(Waypoint*)));
            connect(wpview, SIGNAL(removeWaypoint(Waypoint*)),      this, SLOT(removeWaypoint(Waypoint*)));
            //connect(wpview, SIGNAL(currentWaypointChanged(quint16)), this, SLOT(currentWaypointChanged(quint16)));
            connect(wpview, SIGNAL(changeCurrentWaypoint(quint16)), this, SLOT(currentWaypointEditableChanged(quint16)));
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

void WaypointList::moveUp(Waypoint* wp)
{
    const QList<Waypoint *> &waypoints = WPM->getWaypointEditableList();

    //get the current position of wp in the local storage
    int i;
    for (i = 0; i < waypoints.count(); i++) {
        if (waypoints[i] == wp)
            break;
    }

    // if wp was found and its not the first entry, move it
    if (i < waypoints.count() && i > 0) {
        WPM->moveWaypoint(i, i-1);
    }
}

void WaypointList::moveDown(Waypoint* wp)
{    
    const QList<Waypoint *> &waypoints = WPM->getWaypointEditableList();

    //get the current position of wp in the local storage
    int i;
    for (i = 0; i < waypoints.count(); i++) {
        if (waypoints[i] == wp)
            break;
    }

    // if wp was found and its not the last entry, move it
    if (i < waypoints.count()-1) {
        WPM->moveWaypoint(i, i+1);
    }
}

void WaypointList::removeWaypoint(Waypoint* wp)
{    
        WPM->removeWaypoint(wp->getId());
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
        const QList<Waypoint *> &waypoints = WPM->getWaypointEditableList();
        while(!waypoints.isEmpty()) {
            WaypointEditableView* widget = wpEditableViews.value(waypoints[0], NULL);
            if (widget) {
                widget->remove();
            }
        }
    }
}

void WaypointList::clearWPWidget()
{    
        // Get list
        const QList<Waypoint *> waypoints = WPM->getWaypointEditableList();


        // XXX delete wps as well

        // Clear UI elements
        while(!waypoints.isEmpty()) {
            WaypointEditableView* widget = wpEditableViews.value(waypoints[0], NULL);
            if (widget) {
                widget->remove();
            }
        }
}

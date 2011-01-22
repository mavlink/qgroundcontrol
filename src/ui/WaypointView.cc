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
 *   @brief Displays one waypoint
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include <QDoubleSpinBox>
#include <QDebug>

#include <cmath>    //M_PI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "WaypointView.h"
#include "ui_WaypointView.h"

WaypointView::WaypointView(Waypoint* wp, QWidget* parent) :
    QWidget(parent),
    m_ui(new Ui::WaypointView)
{
    m_ui->setupUi(this);

    this->wp = wp;

    // add actions
    m_ui->comboBox_action->addItem("Navigate",MAV_ACTION_NAVIGATE);
    m_ui->comboBox_action->addItem("TakeOff",MAV_ACTION_TAKEOFF);
    m_ui->comboBox_action->addItem("Land",MAV_ACTION_LAND);
    m_ui->comboBox_action->addItem("Loiter",MAV_ACTION_LOITER);

    // add frames 
    m_ui->comboBox_frame->addItem("Global",MAV_FRAME_GLOBAL);
    m_ui->comboBox_frame->addItem("Local",MAV_FRAME_LOCAL);

    // Read values and set user interface
    updateValues();

    // defaults
    //changedAction(wp->getAction());
    //changedFrame(wp->getFrame());

    connect(m_ui->posNSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setX(double)));
    connect(m_ui->posESpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setY(double)));
    connect(m_ui->posDSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));

    connect(m_ui->lonSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setX(double)));
    connect(m_ui->latSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setY(double)));
    connect(m_ui->altSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));

    //hidden degree to radian conversion of the yaw angle
    connect(m_ui->yawSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setYaw(int)));
    connect(this, SIGNAL(setYaw(double)), wp, SLOT(setYaw(double)));

    connect(m_ui->upButton, SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(m_ui->downButton, SIGNAL(clicked()), this, SLOT(moveDown()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));

    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)), this, SLOT(changedAutoContinue(int)));
    connect(m_ui->selectedBox, SIGNAL(stateChanged(int)), this, SLOT(changedCurrent(int)));
    connect(m_ui->comboBox_action, SIGNAL(activated(int)), this, SLOT(changedAction(int)));
    connect(m_ui->comboBox_frame, SIGNAL(activated(int)), this, SLOT(changedFrame(int)));

    connect(m_ui->orbitSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setOrbit(double)));
    connect(m_ui->holdTimeSpinBox, SIGNAL(valueChanged(int)), wp, SLOT(setHoldTime(int)));
}

void WaypointView::setYaw(int yawDegree)
{
    emit setYaw((double)yawDegree*M_PI/180.);
}

void WaypointView::moveUp()
{
    emit moveUpWaypoint(wp);
}

void WaypointView::moveDown()
{
    emit moveDownWaypoint(wp);
}

void WaypointView::remove()
{
    emit removeWaypoint(wp);
    delete this;
}

void WaypointView::changedAutoContinue(int state)
{
    if (state == 0)
        wp->setAutocontinue(false);
    else
        wp->setAutocontinue(true);
}

void WaypointView::changedAction(int index)
{
    // set action for waypoint


    // hide everything to start
    m_ui->orbitSpinBox->hide();
    m_ui->takeOffAngleSpinBox->hide();
    m_ui->autoContinue->hide();
    m_ui->holdTimeSpinBox->hide();

    // set waypoint action
    MAV_ACTION action = (MAV_ACTION) m_ui->comboBox_action->itemData(index).toUInt();
    wp->setAction(action);

    // expose ui based on action
    switch(action)
    {
    case MAV_ACTION_TAKEOFF:
        m_ui->takeOffAngleSpinBox->show();
        break;
    case MAV_ACTION_LAND:
        break;
    case MAV_ACTION_NAVIGATE:
        m_ui->autoContinue->show();
		m_ui->orbitSpinBox->show();
        break;
    case MAV_ACTION_LOITER:
        m_ui->orbitSpinBox->show();
        m_ui->holdTimeSpinBox->show();
        break;
    default:
        std::cerr << "unknown action" << std::endl;
    }
}

void WaypointView::changedFrame(int index)
{
    // hide everything to start
    m_ui->lonSpinBox->hide();
    m_ui->latSpinBox->hide();
    m_ui->altSpinBox->hide();
    m_ui->posNSpinBox->hide();
    m_ui->posESpinBox->hide();
    m_ui->posDSpinBox->hide();

    // set waypoint action
    MAV_FRAME frame = (MAV_FRAME)m_ui->comboBox_frame->itemData(index).toUInt();
    wp->setFrame(frame);

    switch(frame)
    {
    case MAV_FRAME_GLOBAL:
        m_ui->lonSpinBox->show();
        m_ui->latSpinBox->show();
        m_ui->altSpinBox->show();
        break;
    case MAV_FRAME_LOCAL:
        m_ui->posNSpinBox->show();
        m_ui->posESpinBox->show();
        m_ui->posDSpinBox->show();
        break;
    default:
        std::cerr << "unknown frame" << std::endl;
    }
}

void WaypointView::changedCurrent(int state)
{
    if (state == 0)
    {
        //m_ui->selectedBox->setChecked(true);
        //m_ui->selectedBox->setCheckState(Qt::Checked);
        //wp->setCurrent(false);
    }
    else
    {
        //wp->setCurrent(true);
        emit changeCurrentWaypoint(wp->getId());   //the slot changeCurrentWaypoint() in WaypointList sets all other current flags to false
    }
}

void WaypointView::updateValues()
{
    // Deactivate signals from the WP
    wp->blockSignals(true);
    // update frame
    MAV_FRAME frame = wp->getFrame();
    int frame_index = m_ui->comboBox_frame->findData(frame);
    if (m_ui->comboBox_frame->currentIndex() != frame_index)
    {
        m_ui->comboBox_frame->setCurrentIndex(frame_index);
    }
    switch(frame)
    {
    case(MAV_FRAME_LOCAL):
        m_ui->posNSpinBox->setValue(wp->getX());
        m_ui->posESpinBox->setValue(wp->getY());
        m_ui->posDSpinBox->setValue(wp->getZ());
        break;
    case(MAV_FRAME_GLOBAL):
        m_ui->lonSpinBox->setValue(wp->getX());
        m_ui->latSpinBox->setValue(wp->getY());
        m_ui->altSpinBox->setValue(wp->getZ());
        break;
    }
    changedFrame(frame_index);

    // update action
    MAV_ACTION action = wp->getAction();
    int action_index = m_ui->comboBox_action->findData(action);
    m_ui->comboBox_action->setCurrentIndex(action_index);
    switch(action)
    {
    case MAV_ACTION_TAKEOFF:
        break;
    case MAV_ACTION_LAND:
        break;
    case MAV_ACTION_NAVIGATE:
        break;
    case MAV_ACTION_LOITER:
        break;
    default:
        std::cerr << "unknown action" << std::endl;
    }
    changedAction(action_index);

    m_ui->yawSpinBox->setValue(wp->getYaw()/M_PI*180.);
    m_ui->selectedBox->setChecked(wp->getCurrent());
    m_ui->autoContinue->setChecked(wp->getAutoContinue());
    m_ui->idLabel->setText(QString("%1").arg(wp->getId()));\
    m_ui->orbitSpinBox->setValue(wp->getOrbit());
    m_ui->holdTimeSpinBox->setValue(wp->getHoldTime());
    wp->blockSignals(false);
}

void WaypointView::setCurrent(bool state)
{
    if (state)
    {
        m_ui->selectedBox->setCheckState(Qt::Checked);
    }
    else
    {
         m_ui->selectedBox->setCheckState(Qt::Unchecked);
    }
}

WaypointView::~WaypointView()
{
    delete m_ui;
}

void WaypointView::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

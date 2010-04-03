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
 *
 */

#include <QDoubleSpinBox>
#include <QDebug>

#include "WaypointView.h"
#include "ui_WaypointView.h"

WaypointView::WaypointView(Waypoint* wp, QWidget* parent) :
    QWidget(parent),
    m_ui(new Ui::WaypointView)
{
    m_ui->setupUi(this);

    this->wp = wp;

    // Read values and set user interface
    m_ui->name->setText(wp->name);
    m_ui->xSpinBox->setValue(wp->x);
    m_ui->ySpinBox->setValue(wp->y);
    m_ui->zSpinBox->setValue(wp->z);
    m_ui->yawSpinBox->setValue(wp->yaw);
    m_ui->selectedBox->setChecked(wp->current);
    m_ui->autoContinue->setChecked(wp->autocontinue);

    connect(m_ui->xSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setX(double)));
    connect(m_ui->ySpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setY(double)));
    connect(m_ui->zSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));
    connect(m_ui->yawSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setYaw(double)));

    connect(m_ui->upButton, SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(m_ui->downButton, SIGNAL(clicked()), this, SLOT(moveDown()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));

    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)), this, SLOT(setAutoContinue(int)));
    connect(m_ui->selectedBox, SIGNAL(clicked()), this, SLOT(setCurrent()));
    connect(m_ui->name, SIGNAL(editingFinished()), this, SLOT(setText()));
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

void WaypointView::setAutoContinue(int state)
{
    if (state == 0)
        wp->autocontinue = false;
    else
        wp->autocontinue = true;
    emit waypointUpdated(wp);
}

void WaypointView::removeCurrentCheck()
{
    m_ui->selectedBox->setCheckState(Qt::Unchecked);
}

void WaypointView::setCurrent()
{
    if (m_ui->selectedBox->isChecked())
        emit setCurrentWaypoint(wp);
    else
        m_ui->selectedBox->setCheckState(Qt::Checked);
}

void WaypointView::setText()
{
    wp->name = m_ui->name->text();
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

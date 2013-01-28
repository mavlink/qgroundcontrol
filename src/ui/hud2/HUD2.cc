/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
 * Convetions:
 *
 * - all outer dimensions specified in percents of widget sizes (height,
 *   width or diagonal). Type is qreal.
 * - point with coordinates (0;0) is center of render area.
 * - all classes use their own internal cached values in pixels.
 */

/*
TODO:
- convert all sizes to percents
- dynamic text size for pitch lines
- dynamic mark size for roll and yaw
- dynamic line widths
- split paint in static and dynamic parts
*/

#include <QtGui>

#include "HUD2.h"
#include "UASManager.h"
#include "UAS.h"

HUD2::~HUD2()
{
    //qDebug() << "HUD2 exit 1";
}

HUD2::HUD2(QWidget *parent)
    : QWidget(parent),
      uas(NULL),
      renderType(RENDER_TYPE_NATIVE)
{   
    layout = new QGridLayout(this);

    switch(renderType){
    case RENDER_TYPE_NATIVE:
        render_instance = new HUD2RenderNative(huddata, this),
        btn.setText(tr("Native"));
        break;
    case RENDER_TYPE_OPENGL:
        render_instance = new HUD2RenderGL(huddata, this),
        btn.setText(tr("GL"));
        break;
    case RENDER_TYPE_OFFSCREEN:
        render_instance = new HUD2RenderOffscreen(huddata, this),
        btn.setText(tr("Offscreen"));
        break;
    default:
        break;
    }

    layout->addWidget(render_instance, 0, 0);

    connect(&btn, SIGNAL(clicked()), this, SLOT(switchRender()));
    layout->addWidget(&btn, 1, 0);
    setLayout(layout);

    // Connect with UAS
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)),
            this,                   SLOT(setActiveUAS(UASInterface*)));
    createActions();
    if (UASManager::instance()->getActiveUAS() != NULL)
        setActiveUAS(UASManager::instance()->getActiveUAS());
}


void HUD2::paint(void){
    switch(renderType){
    case RENDER_TYPE_NATIVE:
        ((HUD2RenderNative*)render_instance)->paint();
        break;
    case RENDER_TYPE_OPENGL:
        ((HUD2RenderGL*)render_instance)->paint();
        break;
    case RENDER_TYPE_OFFSCREEN:
        ((HUD2RenderOffscreen*)render_instance)->paint();
        break;
    default:
        break;
    }
}


void HUD2::switchRender(void)
{
    layout->removeWidget(render_instance);
    delete render_instance;

    renderType++;
    if (renderType == RENDER_TYPE_ENUM_END)
        renderType = RENDER_TYPE_NATIVE;

    switch(renderType){
    case RENDER_TYPE_NATIVE:
        render_instance = new HUD2RenderNative(huddata, this),
        btn.setText(tr("Native"));
        break;
    case RENDER_TYPE_OPENGL:
        render_instance = new HUD2RenderGL(huddata, this),
        btn.setText(tr("GL"));
        break;
    case RENDER_TYPE_OFFSCREEN:
        render_instance = new HUD2RenderOffscreen(huddata, this),
        btn.setText(tr("Offscreen"));
        break;
    default:
        break;
    }

    layout->addWidget(render_instance, 0, 0);
    int tmp = render_instance->width();
    render_instance->setGeometry(10,10,10,10);
}


void HUD2::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    if (!isnan(roll) && !isinf(roll) && !isnan(pitch) && !isinf(pitch) && !isnan(yaw) && !isinf(yaw))
    {
        huddata.roll  = roll;
        huddata.pitch = pitch;
        huddata.yaw   = yaw;
        this->paint();
    }
}


void HUD2::updateGlobalPosition(UASInterface* uas, double lat, double lon, double altitude, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);
    huddata.lat = lat;
    huddata.lon = lon;
    huddata.alt = altitude;
}


/**
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HUD2::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL) {
        // Disconnect any previously connected active MAV
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
        disconnect(this->uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString)));
        disconnect(this->uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(this->uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));

        disconnect(this->uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int, int)));
    }

    if (uas) {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(uas, SIGNAL(batteryChanged(UASInterface*, double, double, int)), this, SLOT(updateBattery(UASInterface*, double, double, int)));
        connect(uas, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString)));
        connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        connect(uas, SIGNAL(heartbeat(UASInterface*)), this, SLOT(receiveHeartbeat(UASInterface*)));

        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(waypointSelected(int,int)), this, SLOT(selectWaypoint(int, int)));

        // Set new UAS
        this->uas = uas;
    }
}

void HUD2::createActions()
{
//    enableHUDAction = new QAction(tr("Enable HUD"), this);
//    enableHUDAction->setStatusTip(tr("Show the HUD instruments in this window"));
//    enableHUDAction->setCheckable(true);
//    enableHUDAction->setChecked(hudInstrumentsEnabled);
//    connect(enableHUDAction, SIGNAL(triggered(bool)), this, SLOT(enableHUDInstruments(bool)));

//    enableVideoAction = new QAction(tr("Enable Video Live feed"), this);
//    enableVideoAction->setStatusTip(tr("Show the video live feed"));
//    enableVideoAction->setCheckable(true);
//    enableVideoAction->setChecked(videoEnabled);
//    connect(enableVideoAction, SIGNAL(triggered(bool)), this, SLOT(enableVideo(bool)));

//    selectOfflineDirectoryAction = new QAction(tr("Select image log"), this);
//    selectOfflineDirectoryAction->setStatusTip(tr("Load previously logged images into simulation / replay"));
//    connect(selectOfflineDirectoryAction, SIGNAL(triggered()), this, SLOT(selectOfflineDirectory()));
}

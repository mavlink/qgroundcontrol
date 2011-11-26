/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of QGCRemoteControlView
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 *   @author Bryan Godbolt <godbolt@ece.ualberta.ca>
 */

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "QGCRemoteControlView.h"
#include "ui_QGCRemoteControlView.h"
#include "UASManager.h"

QGCRemoteControlView::QGCRemoteControlView(QWidget *parent) :
    QWidget(parent),
    uasId(-1),
    rssi(0.0f),
    updated(false),
    channelLayout(new QVBoxLayout()),
    ui(NULL)
{
    ui->setupUi(this);
    QGridLayout* layout = new QGridLayout(this);
    layout->addLayout(channelLayout, 1, 0, 1, 2);
    nameLabel = new QLabel(this);
    layout->addWidget(nameLabel, 0, 0, 1, 2);

    this->setVisible(false);
    //setVisible(false);

    calibrate = new QPushButton(tr("Calibrate"), this);
    QHBoxLayout *calibrateButtonLayout = new QHBoxLayout();
    calibrateButtonLayout->addWidget(calibrate, 0, Qt::AlignHCenter);
    layout->addItem(calibrateButtonLayout, 3, 0, 1, 2);

    calibrationWindow = new RadioCalibrationWindow(this);
    connect(calibrate, SIGNAL(clicked()), calibrationWindow, SLOT(show()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(int)), this, SLOT(setUASId(int)));

    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(redraw()));
    updateTimer.start(1500);
}

QGCRemoteControlView::~QGCRemoteControlView()
{
	if(this->ui)
	{
		delete ui;
	}
	if(this->channelLayout)
	{
		delete channelLayout;
	}
}

void QGCRemoteControlView::setUASId(int id)
{
    if (uasId != -1)
    {
        UASInterface* uas = UASManager::instance()->getUASForId(uasId);
        if (uas)
        {
            // The UAS exists, disconnect any existing connections
            disconnect(uas, SIGNAL(remoteControlChannelRawChanged(int,float,float)), this, SLOT(setChannel(int,float,float)));
            disconnect(uas, SIGNAL(remoteControlRSSIChanged(float)), this, SLOT(setRemoteRSSI(float)));
            disconnect(uas, SIGNAL(radioCalibrationRawReceived(const QPointer<RadioCalibrationData>&)), calibrationWindow, SLOT(receive(const QPointer<RadioCalibrationData>&)));
            disconnect(uas, SIGNAL(remoteControlChannelRawChanged(int,float)), calibrationWindow, SLOT(setChannel(int,float)));
            disconnect(uas, SIGNAL(remoteControlChannelScaledChanged(int,float)), this, SLOT(setChannelScaled(int,float)));
        }
    }

    // Clear channel count
    raw.clear();
    raw.resize(0);
    normalized.clear();
    normalized.resize(0);

    foreach (QLabel* label, rawLabels)
    {
        label->deleteLater();
    }

    foreach(QProgressBar* bar, progressBars)
    {
        bar->deleteLater();
    }

    rawLabels.clear();
    rawLabels.resize(0);
    progressBars.clear();
    progressBars.resize(0);

    // Connect the new UAS
    UASInterface* newUAS = UASManager::instance()->getUASForId(id);
    if (newUAS)
    {
        // New UAS exists, connect
        nameLabel->setText(QString("RC Input of %1").arg(newUAS->getUASName()));
        calibrationWindow->setUASId(id);
        connect(newUAS, SIGNAL(radioCalibrationReceived(const QPointer<RadioCalibrationData>&)), calibrationWindow, SLOT(receive(const QPointer<RadioCalibrationData>&)));

        connect(newUAS, SIGNAL(remoteControlRSSIChanged(float)), this, SLOT(setRemoteRSSI(float)));
        connect(newUAS, SIGNAL(remoteControlChannelRawChanged(int,float)), this, SLOT(setChannelRaw(int,float)));
        connect(newUAS, SIGNAL(remoteControlChannelScaledChanged(int,float)), this, SLOT(setChannelScaled(int,float)));

        // only connect raw channels to calibration window widget
        connect(newUAS, SIGNAL(remoteControlChannelRawChanged(int,float)), calibrationWindow, SLOT(setChannel(int,float)));
    }
}

void QGCRemoteControlView::setChannelRaw(int channelId, float raw)
{

    if (this->raw.size() <= channelId) {
        // This is a new channel, append it
        this->raw.append(raw);
        //this->normalized.append(0);
        appendChannelWidget(channelId);
        updated = true;
    } else {
        // This is an existing channel, aupdate it
        if (this->raw[channelId] != raw) updated = true;
        this->raw[channelId] = raw;
    }
}

void QGCRemoteControlView::setChannelScaled(int channelId, float normalized)
{
    if (this->normalized.size() <= channelId) // using raw vector as size indicator
    {
        // This is a new channel, append it
        this->normalized.append(normalized);
        appendChannelWidget(channelId);
        updated = true;
    }
    else
    {
        // This is an existing channel, update it
        if (this->normalized[channelId] != normalized) updated = true;
        this->normalized[channelId] = normalized;
    }
}

void QGCRemoteControlView::setRemoteRSSI(float rssiNormalized)
{
    if (rssi != rssiNormalized) updated = true;
    rssi = rssiNormalized;
}

void QGCRemoteControlView::appendChannelWidget(int channelId)
{
    // Create new layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    // Add content
    layout->addWidget(new QLabel(QString("Channel %1").arg(channelId + 1), this));
    QLabel* raw = new QLabel(this);

    // Append raw label
    rawLabels.append(raw);
    layout->addWidget(raw);
    // Append progress bar
    QProgressBar* normalized = new QProgressBar(this);
    normalized->setMinimum(-100);
    normalized->setMaximum(100);
    normalized->setFormat("%v%");
    progressBars.append(normalized);
    layout->addWidget(normalized);
    channelLayout->addLayout(layout);
}

void QGCRemoteControlView::redraw()
{
    if(isVisible() && updated)
    {
        // Update raw values
        //for(int i = 0; i < rawLabels.count(); i++)
        //{
        //rawLabels.at(i)->setText(QString("%1 us").arg(raw.at(i), 4, 10, QChar('0')));
        //}

        // Update percent bars
        for(int i = 0; i < progressBars.count(); i++)
        {
            rawLabels.at(i)->setText(QString("%1 us").arg(raw.at(i), 4, 10, QChar('0')));
            int vv = normalized.at(i)*100.0f;
            //progressBars.at(i)->setValue(vv);
//            int vv = raw.at(i)*1.0f;
            progressBars.at(i)->setValue(vv);
        }
        // Update RSSI
        if(rssi>0) {
            //rssiBar->setValue(rssi);//*100);
        }

        updated = false;
    }
}

void QGCRemoteControlView::changeEvent(QEvent *e)
{
    Q_UNUSED(e);
    // FIXME If the lines below are commented in
    // runtime errors can occur on x64 systems.
//    QWidget::changeEvent(e);
//    switch (e->type()) {
//    case QEvent::LanguageChange:
//        //ui->retranslateUi(this);
//        break;
//    default:
//        break;
//    }
}

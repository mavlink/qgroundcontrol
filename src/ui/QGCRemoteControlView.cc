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
    ui(new Ui::QGCRemoteControlView)
{

    //ui->setupUi(this);
    QGridLayout* layout = new QGridLayout(this);
    layout->addLayout(channelLayout, 1, 0, 1, 2);
    // Name label
    nameLabel = new QLabel(this);
    nameLabel->setText("No MAV selected yet..");
    layout->addWidget(nameLabel, 0, 0, 1, 2);

    // RSSI bar
    // Create new layout
    QHBoxLayout* rssiLayout = new QHBoxLayout();
    rssiLayout->setSpacing(5);
    // Add content
    rssiLayout->addWidget(new QLabel(tr("Signal"), this));
    // Append raw label
    // Append progress bar
    rssiBar = new QProgressBar(this);
    rssiBar->setMinimum(0);
    rssiBar->setMaximum(100);
    rssiBar->setValue(0);
    rssiLayout->addWidget(rssiBar);
    layout->addItem(rssiLayout, 2, 0, 1, 2);
    setVisible(false);

    calibrate = new QPushButton(tr("Calibrate"), this);
    QHBoxLayout *calibrateButtonLayout = new QHBoxLayout();
    calibrateButtonLayout->addWidget(calibrate, 0, Qt::AlignHCenter);
    layout->addItem(calibrateButtonLayout, 3, 0, 1, 2);

    calibrationWindow = new RadioCalibrationWindow(this);
    connect(calibrate, SIGNAL(clicked()), calibrationWindow, SLOT(show()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(int)), this, SLOT(setUASId(int)));
}

QGCRemoteControlView::~QGCRemoteControlView()
{
    delete ui;
    delete channelLayout;
}

void QGCRemoteControlView::setUASId(int id)
{
    if (uasId != -1)
    {
        UASInterface* uas = UASManager::instance()->getUASForId(id);
        if (uas)
        {
            // The UAS exists, disconnect any existing connections
            disconnect(uas, SIGNAL(remoteControlChannelChanged(int,float,float)), this, SLOT(setChannel(int,float,float)));
            disconnect(uas, SIGNAL(remoteControlRSSIChanged(float)), this, SLOT(setRemoteRSSI(float)));
            disconnect(uas, SIGNAL(radioCalibrationReceived(const QPointer<RadioCalibrationData>)), calibrationWindow, SLOT(receive(const QPointer<RadioCalibrationData>&)));
            disconnect(uas, SIGNAL(remoteControlChannelChanged(int,float,float)), calibrationWindow, SLOT(setChannel(int,float,float)));
        }
    }

    // Connect the new UAS
    UASInterface* newUAS = UASManager::instance()->getUASForId(id);
    if (newUAS)
    {
        // New UAS exists, connect
        nameLabel->setText(QString("RC Input of %1").arg(newUAS->getUASName()));
        calibrationWindow->setUASId(id);
        connect(newUAS, SIGNAL(radioCalibrationReceived(const QPointer<RadioCalibrationData>)), calibrationWindow, SLOT(receive(const QPointer<RadioCalibrationData>&)));
        connect(newUAS, SIGNAL(remoteControlChannelChanged(int,float,float)), this, SLOT(setChannel(int,float,float)));
        connect(newUAS, SIGNAL(remoteControlChannelChanged(int,float,float)), calibrationWindow, SLOT(setChannel(int,float,float)));
        connect(newUAS, SIGNAL(remoteControlRSSIChanged(float)), this, SLOT(setRemoteRSSI(float)));
    }
}

void QGCRemoteControlView::setChannel(int channelId, float raw, float normalized)
{
    if (this->raw.size() <= channelId)
    {
        // This is a new channel, append it
        this->raw.append(raw);
        this->normalized.append(normalized);
        appendChannelWidget(channelId);
    }
    else
    {
        // This is an existing channel, update it
        this->raw[channelId] = raw;
        this->normalized[channelId] = normalized;
    }
    updated = true;

    // FIXME Will be timer based in the future
    redraw();
}

void QGCRemoteControlView::setRemoteRSSI(float rssiNormalized)
{
    rssi = rssiNormalized;
    updated = true;
}

void QGCRemoteControlView::appendChannelWidget(int channelId)
{
    // Create new layout
    QHBoxLayout* layout = new QHBoxLayout();
    // Add content
    layout->addWidget(new QLabel(QString("Channel %1").arg(channelId + 1), this));
    QLabel* raw = new QLabel(this);
    // Append raw label
    rawLabels.append(raw);
    layout->addWidget(raw);
    // Append progress bar
    QProgressBar* normalized = new QProgressBar(this);
    normalized->setMinimum(0);
    normalized->setMaximum(100);
    progressBars.append(normalized);
    layout->addWidget(normalized);
    channelLayout->addLayout(layout);
}

void QGCRemoteControlView::redraw()
{
    if(isVisible() && updated)
    {
        // Update raw values
        for(int i = 0; i < rawLabels.count(); i++)
        {
            rawLabels.at(i)->setText(QString("%1 us").arg(raw.at(i)));
        }

        // Update percent bars
        for(int i = 0; i < progressBars.count(); i++)
        {
            progressBars.at(i)->setValue(normalized.at(i)*100.0f);
        }
        updated = false;
    }
}

void QGCRemoteControlView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

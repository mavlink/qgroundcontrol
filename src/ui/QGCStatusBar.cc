/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2013 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include <QToolButton>
#include <QLabel>
#include <QSpacerItem>
#include <QFileDialog>

#include "QGCStatusBar.h"
#include "UASManager.h"
#include "MainWindow.h"
#include "QGCApplication.h"

QGCStatusBar::QGCStatusBar(QWidget *parent) :
    QStatusBar(parent),
    player(NULL),
    changed(true),
    lastLogDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
{
    setObjectName("QGC_STATUSBAR");
    loadSettings();
}

void QGCStatusBar::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);
    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_PanelStatusBar, &opt, &p, this);
}

void QGCStatusBar::setLogPlayer(QGCMAVLinkLogPlayer* player)
{
    this->player = player;
    addPermanentWidget(player);
}

void QGCStatusBar::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINKLOGPLAYER");
    lastLogDirectory = settings.value("LAST_LOG_DIRECTORY", lastLogDirectory).toString();
    settings.endGroup();
}

void QGCStatusBar::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINKLOGPLAYER");
    settings.setValue("LAST_LOG_DIRECTORY", lastLogDirectory);
    settings.endGroup();
    settings.sync();
}

QGCStatusBar::~QGCStatusBar()
{
    storeSettings();
}

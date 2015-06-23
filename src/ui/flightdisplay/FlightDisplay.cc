/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief QGC Main Flight Display
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>

#include "ScreenToolsController.h"
#include "FlightDisplay.h"
#include "UASManager.h"

const char* kMainFlightDisplayGroup = "MainFlightDisplay";

FlightDisplay::FlightDisplay(QWidget *parent)
    : QGCQmlWidgetHolder(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setObjectName("MainFlightDisplay");
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
#ifndef __android__
    setMinimumWidth( 31 * ScreenToolsController::defaultFontPixelSize_s());
    setMinimumHeight(33 * ScreenToolsController::defaultFontPixelSize_s());
#endif
    setContextPropertyObject("flightDisplay", this);
    setSource(QUrl::fromUserInput("qrc:/qml/FlightDisplay.qml"));
    setVisible(true);
}

FlightDisplay::~FlightDisplay()
{
}

void FlightDisplay::saveSetting(const QString &name, const QString& value)
{
    QSettings settings;
    QString key(kMainFlightDisplayGroup);
    key += "/" + name;
    settings.setValue(key, value);
}

QString FlightDisplay::loadSetting(const QString &name, const QString& defaultValue)
{
    QSettings settings;
    QString key(kMainFlightDisplayGroup);
    key += "/" + name;
    return settings.value(key, defaultValue).toString();
}

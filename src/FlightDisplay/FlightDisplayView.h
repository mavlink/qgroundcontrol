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

#ifndef FlightDisplayView_H
#define FlightDisplayView_H

#include "QGCQmlWidgetHolder.h"

class FlightDisplayView : public QGCQmlWidgetHolder
{
    Q_OBJECT
public:
    FlightDisplayView(QWidget* parent = NULL);
    ~FlightDisplayView();

    /// @brief Invokes the Flight Display Options menu
    void showOptionsMenu() { emit showOptionsMenuChanged(); }

    Q_PROPERTY(bool hasVideo READ hasVideo CONSTANT)

#if defined(QGC_GST_STREAMING)
    bool    hasVideo            () { return true; }
#else
    bool    hasVideo            () { return false; }
#endif

signals:
    void showOptionsMenuChanged ();

};

#endif

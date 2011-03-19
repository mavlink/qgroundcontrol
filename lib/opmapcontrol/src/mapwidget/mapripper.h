/**
******************************************************************************
*
* @file       mapripper.h
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A class that allows ripping of a selection of the map
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
*
*****************************************************************************/
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#ifndef MAPRIPPER_H
#define MAPRIPPER_H

#include <QThread>
#include "../internals/core.h"
#include "mapripform.h"
#include <QObject>
#include <QMessageBox>
namespace mapcontrol
{
    class MapRipper:public QThread
    {
        Q_OBJECT
    public:
        MapRipper(internals::Core *,internals::RectLatLng const&);
        void run();
    private:
        QList<core::Point> points;
        int zoom;
        core::MapType::Types type;
        int sleep;
        internals::RectLatLng area;
        bool cancel;
        MapRipForm * progressForm;
        int maxzoom;
        internals::Core * core;

    signals:
        void percentageChanged(int const& perc);
        void numberOfTilesChanged(int const& total,int const& actual);
        void providerChanged(QString const& prov,int const& zoom);


    public slots:
        void finish();
    };
}
#endif // MAPRIPPER_H

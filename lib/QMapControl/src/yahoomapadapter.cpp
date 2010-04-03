/*
*
* This file is part of QMapControl,
* an open-source cross-platform map widget
*
* Copyright (C) 2007 - 2008 Kai Winter
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with QMapControl. If not, see <http://www.gnu.org/licenses/>.
*
* Contact e-mail: kaiwinter@gmx.de
* Program URL   : http://qmapcontrol.sourceforge.net/
*
*/

#include "yahoomapadapter.h"
namespace qmapcontrol
{
    YahooMapAdapter::YahooMapAdapter()
        : TileMapAdapter("png.maps.yimg.com", "/png?v=3.1.0&x=%2&y=%3&z=%1", 256, 17,0)
    {
        int zoom = max_zoom < min_zoom ? min_zoom - current_zoom : current_zoom;
        numberOfTiles = pow(2, zoom+1);
    }
    YahooMapAdapter::YahooMapAdapter(QString host, QString url)
        : TileMapAdapter(host, url, 256, 17,0)
    {
        int zoom = max_zoom < min_zoom ? min_zoom - current_zoom : current_zoom;
        numberOfTiles = pow(2, zoom+1);
    }
    YahooMapAdapter::~YahooMapAdapter()
    {
    }

    bool YahooMapAdapter::isValid(int /*x*/, int /*y*/, int /*z*/) const
    {
        return true;
    }

    int YahooMapAdapter::tilesonzoomlevel(int zoomlevel) const
    {
        return int(pow(2, zoomlevel+1));
    }

    int YahooMapAdapter::yoffset(int y) const
    {
        int zoom = max_zoom < min_zoom ? min_zoom - current_zoom : current_zoom;

        int tiles = int(pow(2, zoom));
        y = y*(-1)+tiles-1;
        return int(y);
    }
}

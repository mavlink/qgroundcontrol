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

#include "mapadapter.h"
namespace qmapcontrol
{
	MapAdapter::MapAdapter(const QString& host, const QString& serverPath, int tilesize, int minZoom, int maxZoom)
	:myhost(host), serverPath(serverPath), mytilesize(tilesize), min_zoom(minZoom), max_zoom(maxZoom)
	{	
		current_zoom = min_zoom;
		loc = QLocale(QLocale::English);
	}

	MapAdapter::~MapAdapter()
	{
	}

	QString MapAdapter::host() const
	{
		return myhost;
	}

	int MapAdapter::tilesize() const
	{
		return mytilesize;
	}

	int MapAdapter::minZoom() const
	{
		return min_zoom;
	}

	int MapAdapter::maxZoom() const
	{
		return max_zoom;
	}

	int MapAdapter::currentZoom() const
	{
		return current_zoom;
	}

	int MapAdapter::adaptedZoom() const
	{
		return max_zoom < min_zoom ? min_zoom - current_zoom : current_zoom;
	}
}

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

#ifndef GOOGLESATMAPADAPTER_H
#define GOOGLESATMAPADAPTER_H

#include "mapadapter.h"
namespace qmapcontrol
{
//! MapAdapter for Google
/*!
 * This is a conveniece class, which extends and configures a TileMapAdapter
 *	@author Kai Winter <kaiwinter@gmx.de>
*/
class GoogleSatMapAdapter : public MapAdapter
{
	Q_OBJECT
	public:
		//! constructor
		/*!
		 * This construct a Google Adapter
		 */
		GoogleSatMapAdapter();
		virtual ~GoogleSatMapAdapter();

		virtual QPoint		coordinateToDisplay(const QPointF&) const;
		virtual QPointF	displayToCoordinate(const QPoint&) const;

		//! returns the host of this MapAdapter
		/*!
		 * @return  the host of this MapAdapter
		 */
		QString	getHost		() const;


	protected:
		virtual void zoom_in();
		virtual void zoom_out();
		virtual QString query(int x, int y, int z) const;
		virtual bool isValid(int x, int y, int z) const;

	private:
		virtual QString getQ(qreal longitude, qreal latitude, int zoom) const;
		qreal getMercatorLatitude(qreal YCoord) const;
		qreal getMercatorYCoord(qreal lati) const;

		qreal coord_per_x_tile;
		qreal coord_per_y_tile;
		int srvNum;
};
}
#endif

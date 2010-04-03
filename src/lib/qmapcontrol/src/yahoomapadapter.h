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

#ifndef YAHOOMAPADAPTER_H
#define YAHOOMAPADAPTER_H

#include "tilemapadapter.h"

namespace qmapcontrol
{
	//! MapAdapter for Yahoo Maps
	/*!
	 *	@author Kai Winter <kaiwinter@gmx.de>
 	 */
	class YahooMapAdapter : public TileMapAdapter
	{
		Q_OBJECT
		public:
		//! constructor
		/*! 
		 * This construct a Yahoo Adapter
		 */
			YahooMapAdapter();
			YahooMapAdapter(QString host, QString url);
			virtual ~YahooMapAdapter();
			bool isValid(int x, int y, int z) const;
		protected:
			virtual int tilesonzoomlevel(int zoomlevel) const;
			virtual int yoffset(int y) const;
	};
}
#endif

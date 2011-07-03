/**
 ******************************************************************************
 *
 * @file       homelocationutil.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Utilities to find the location of openpilot GCS files:
 *             - Plugins Share directory path
 *
 * @brief      Home location utility functions
 *
 * @see        The GNU Public License (GPL) Version 3
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

#include "homelocationutil.h"

#include <stdint.h>
#include <QDebug>
#include <QDateTime>

#include "coordinateconversions.h"
#include "worldmagmodel.h"

namespace Utils {

	HomeLocationUtil::HomeLocationUtil()
    {
//        Initialize();
    }

	// input params: LLA
	//
	// output params: ECEF, RNE and Be
	int HomeLocationUtil::getDetails(double LLA[3], double ECEF[3], double RNE[9], double Be[3])
    {
		// *************
		// check input parms

		double latitude = LLA[0];
		double longitude = LLA[1];
		double altitude = LLA[2];

		if (latitude != latitude) return -1;				// prevent nan error
		if (longitude != longitude) return -2;				// prevent nan error
		if (altitude != altitude) return -3;				// prevent nan error

		if (latitude < -90 || latitude > 90) return -4;		// range checking
		if (longitude < -180 || longitude > 180) return -5;	// range checking

		// *************

		QDateTime dt = QDateTime::currentDateTime().toUTC();

		CoordinateConversions().LLA2ECEF(LLA, ECEF);
		CoordinateConversions().RneFromLLA(LLA, (double (*)[3])RNE);
		if (WorldMagModel().GetMagVector(LLA, dt.date().month(), dt.date().day(), dt.date().year(), Be) < 0)
			return -6;

		return 0;	// OK
    }

}

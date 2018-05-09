/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QLOCATIONUTILS_P_H
#define QLOCATIONUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE
class QTime;
class QByteArray;

class QGeoPositionInfo;
class QLocationUtils
{
public:
    inline static bool isValidLat(double lat) {
        return lat >= -90 && lat <= 90;
    }
    inline static bool isValidLong(double lng) {
        return lng >= -180 && lng <= 180;
    }

    inline static double clipLat(double lat) {
        if (lat > 90)
            lat = 90;
        else if (lat < -90)
            lat = -90;
        return lat;
    }

    inline static double wrapLong(double lng) {
        if (lng > 180)
            lng -= 360;
        else if (lng < -180)
            lng += 360;
        return lng;
    }

    /*
        Creates a QGeoPositionInfo from a GGA, GLL, RMC, VTG or ZDA sentence.

        Note:
        - GGA and GLL sentences have time but not date so the update's
          QDateTime object will have an invalid date.
        - RMC reports date with a two-digit year so in this case the year
          is assumed to be after the year 2000.
    */
    Q_AUTOTEST_EXPORT static bool getPosInfoFromNmea(const char *data, int size,
                                                     QGeoPositionInfo *info, double uere,
                                                     bool *hasFix = 0);

    /*
        Returns true if the given NMEA sentence has a valid checksum.
    */
    Q_AUTOTEST_EXPORT static bool hasValidNmeaChecksum(const char *data, int size);

    /*
        Returns time from a string in hhmmss or hhmmss.z+ format.
    */
    Q_AUTOTEST_EXPORT static bool getNmeaTime(const QByteArray &bytes, QTime *time);

    /*
        Accepts for example ("2734.7964", 'S', "15306.0124", 'E') and returns the
        lat-long values. Fails if lat or long fail isValidLat() or isValidLong().
    */
    Q_AUTOTEST_EXPORT static bool getNmeaLatLong(const QByteArray &latString, char latDirection, const QByteArray &lngString, char lngDirection, double *lat, double *lon);
};

QT_END_NAMESPACE

#endif

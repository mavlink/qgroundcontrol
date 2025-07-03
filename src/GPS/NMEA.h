/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

class NMEAMessage
{
public:
    NMEAMessage(const QGeoCoordinate &coordinate);
    QString getGGA() const;

private:
    QString _getCheckSum(const QString &line) const;
    const QGeoCoordinate _coordinate;
};

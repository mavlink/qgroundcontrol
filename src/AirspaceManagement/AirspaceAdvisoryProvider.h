/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @file AirspaceAdvisoryProvider.h
 * Weather information provided by the Airspace Managemement
 */

#include <QObject>
#include <QGeoCoordinate>

class AirspaceAdvisoryProvider : public QObject
{
    Q_OBJECT
public:
    AirspaceAdvisoryProvider            (QObject *parent = nullptr);
    virtual ~AirspaceAdvisoryProvider   () {}

    Q_PROPERTY(bool     valid           READ valid          NOTIFY advisoryChanged)

    virtual bool    valid           ()  = 0;    ///< Current data is valid

    /**
     * Set region of interest that should be queried. When finished, the advisoryChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void setROI (const QGeoCoordinate& center, double radiusMeters) = 0;

signals:
    void advisoryChanged  ();
};

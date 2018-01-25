/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"

#include "AirspaceAdvisoryProvider.h"
#include "AirMapSharedState.h"

#include <QGeoCoordinate>

#include "airmap/status.h"

/**
 * @file AirMapAdvisories.h
 * Advisory information provided by AirMap.
 */

class AirMapAdvisories : public AirspaceAdvisoryProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapAdvisories            (AirMapSharedState &shared, QObject *parent = nullptr);

    bool        valid           () override { return _valid; }

    void        setROI          (const QGeoCoordinate& center, double radiusMeters) override;

signals:
    void        error           (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    void        _requestAdvisories  (const QGeoCoordinate& coordinate, double radiusMeters);

private:
    bool                _valid;
    AirMapSharedState&  _shared;
    QGeoCoordinate      _lastRoiCenter;
    airmap::Status::Color                   _advisory_color;
    std::vector<airmap::Status::Advisory>   _advisories;
};

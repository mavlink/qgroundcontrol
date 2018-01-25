/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapAdvisories.h"
#include "AirMapManager.h"

#define ADVISORY_UPDATE_DISTANCE    500     //-- 500m threshold for updates

using namespace airmap;

AirMapAdvisories::AirMapAdvisories(AirMapSharedState& shared, QObject *parent)
    : AirspaceAdvisoryProvider(parent)
    , _valid(false)
    , _shared(shared)
{
}

void
AirMapAdvisories::setROI(const QGeoCoordinate& center, double radiusMeters)
{
    //-- If first time or we've moved more than ADVISORY_UPDATE_DISTANCE, ask for updates.
    if(!_lastRoiCenter.isValid() || _lastRoiCenter.distanceTo(center) > ADVISORY_UPDATE_DISTANCE) {
        _lastRoiCenter = center;
        _requestAdvisories(center, radiusMeters);
    }
}

void
AirMapAdvisories::_requestAdvisories(const QGeoCoordinate& coordinate, double radiusMeters)
{
    qCDebug(AirMapManagerLog) << "Advisories Request";
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Advisories";
        _valid = false;
        emit advisoryChanged();
        return;
    }
    _advisories.clear();
    _valid = false;
    Status::GetStatus::Parameters params;
    params.longitude = coordinate.longitude();
    params.latitude  = coordinate.latitude();
    params.weather   = false;
    params.buffer    = radiusMeters;
    _shared.client()->status().get_status_by_point(params, [this, coordinate](const Status::GetStatus::Result& result) {
        if (result) {
            qCDebug(AirMapManagerLog) << _advisories.size() << "Advisories Received";
            _advisories     = result.value().advisories;
            _advisory_color = result.value().advisory_color;
            if(_advisories.size()) {
                _valid = true;
                qCDebug(AirMapManagerLog) << "Advisory Info: " << _advisories.size() << _advisories[0].airspace.name().data();
            }
        } else {
            qCDebug(AirMapManagerLog) << "Advisories Request Failed";
            _valid = false;
        }
        emit advisoryChanged();
    });
}

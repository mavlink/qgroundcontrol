/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapAdvisoryManager.h"
#include "AirspaceRestriction.h"
#include "AirMapManager.h"

#include <QTimer>

#include "airmap/airspaces.h"

#define ADVISORY_UPDATE_DISTANCE    500     //-- 500m threshold for updates

using namespace airmap;

//-----------------------------------------------------------------------------
AirMapAdvisory::AirMapAdvisory(QObject* parent)
    : AirspaceAdvisory(parent)
    , _radius(0.0)
{
}

//-----------------------------------------------------------------------------
AirMapAdvisoryManager::AirMapAdvisoryManager(AirMapSharedState& shared, QObject *parent)
    : AirspaceAdvisoryProvider(parent)
    , _valid(false)
    , _lastRadius(0.0)
    , _shared(shared)
{
}

//-----------------------------------------------------------------------------
void
AirMapAdvisoryManager::setROI(const QGeoCoordinate& center, double radiusMeters)
{
    //-- If first time or we've moved more than ADVISORY_UPDATE_DISTANCE, ask for updates.
    if(!_lastRoiCenter.isValid() || _lastRoiCenter.distanceTo(center) > ADVISORY_UPDATE_DISTANCE || _lastRadius != radiusMeters) {
        _lastRadius    = radiusMeters;
        _lastRoiCenter = center;
        _requestAdvisories();
    }
}

//-----------------------------------------------------------------------------
static bool
adv_sort(QObject* a, QObject* b)
{
    AirMapAdvisory* aa = qobject_cast<AirMapAdvisory*>(a);
    AirMapAdvisory* bb = qobject_cast<AirMapAdvisory*>(b);
    if(!aa || !bb) return false;
    return (int)aa->color() > (int)bb->color();
}

//-----------------------------------------------------------------------------
void
AirMapAdvisoryManager::_requestAdvisories()
{
    qCDebug(AirMapManagerLog) << "Advisories Request";
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Advisories";
        _valid = false;
        emit advisoryChanged();
        return;
    }
    _valid = false;
    _airspaces.clearAndDeleteContents();
    Status::GetStatus::Parameters params;
    params.longitude = _lastRoiCenter.longitude();
    params.latitude  = _lastRoiCenter.latitude();
    params.types     = Airspace::Type::all;
    params.weather   = false;
    params.buffer    = _lastRadius;
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->status().get_status_by_point(params, [this, isAlive](const Status::GetStatus::Result& result) {
        if (!isAlive.lock()) return;
        if (result) {
            qCDebug(AirMapManagerLog) << "Successful advisory search. Items:" << result.value().advisories.size();
            _airspaceColor = (AirspaceAdvisoryProvider::AdvisoryColor)(int)result.value().advisory_color;
            const std::vector<Status::Advisory> advisories = result.value().advisories;
            for (const auto& advisory : advisories) {
                AirMapAdvisory* pAdvisory = new AirMapAdvisory(this);
                pAdvisory->_id          = QString::fromStdString(advisory.airspace.id());
                pAdvisory->_name        = QString::fromStdString(advisory.airspace.name());
                pAdvisory->_type        = (AirspaceAdvisory::AdvisoryType)(int)advisory.airspace.type();
                pAdvisory->_color       = (AirspaceAdvisoryProvider::AdvisoryColor)(int)advisory.color;
                _airspaces.append(pAdvisory);
                qCDebug(AirMapManagerLog) << "Adding advisory" << pAdvisory->name();
            }
            //-- Sort in order of color (priority)
            std::sort(_airspaces.objectList()->begin(), _airspaces.objectList()->end(), adv_sort);
            _valid = true;
        } else {
            qCDebug(AirMapManagerLog) << "Advisories Request Failed";
        }
        emit advisoryChanged();
    });
}

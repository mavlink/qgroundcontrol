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
{
}

//-----------------------------------------------------------------------------
AirMapAdvisoryManager::AirMapAdvisoryManager(AirMapSharedState& shared, QObject *parent)
    : AirspaceAdvisoryProvider(parent)
    , _valid(false)
    , _shared(shared)
{
}

//-----------------------------------------------------------------------------
void
AirMapAdvisoryManager::setROI(const QGCGeoBoundingCube& roi)
{
    //-- If first time or we've moved more than ADVISORY_UPDATE_DISTANCE, ask for updates.
    if(!_lastROI.isValid() || _lastROI.pointNW.distanceTo(roi.pointNW) > ADVISORY_UPDATE_DISTANCE || _lastROI.pointSE.distanceTo(roi.pointSE) > ADVISORY_UPDATE_DISTANCE) {
        _lastROI = roi;
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
    _advisories.clearAndDeleteContents();
    Status::GetStatus::Parameters params;
    params.longitude = _lastROI.center().longitude();
    params.latitude  = _lastROI.center().latitude();
    params.types     = Airspace::Type::all;
    params.weather   = false;
    double diagonal  = _lastROI.pointNW.distanceTo(_lastROI.pointSE);
    params.buffer    = fmax(fmin(diagonal, 10000.0), 500.0);
    params.flight_date_time = Clock::universal_time();
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
                _advisories.append(pAdvisory);
                qCDebug(AirMapManagerLog) << "Adding advisory" << pAdvisory->name();
            }
            //-- Sort in order of color (priority)
            _advisories.beginReset();
            std::sort(_advisories.objectList()->begin(), _advisories.objectList()->end(), adv_sort);
            _advisories.endReset();
            _valid = true;
        } else {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            qCDebug(AirMapManagerLog) << "Advisories Request Failed" << QString::fromStdString(result.error().message()) << description;
        }
        emit advisoryChanged();
    });
}

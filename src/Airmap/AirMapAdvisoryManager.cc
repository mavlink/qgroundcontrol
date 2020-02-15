/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapAdvisoryManager.h"
#include "AirspaceRestriction.h"
#include "AirMapRulesetsManager.h"
#include "AirMapManager.h"
#include "QGCApplication.h"

#include <cmath>
#include <QTimer>
#include <QDateTime>

#include "airmap/airspaces.h"
#include "airmap/advisory.h"

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
AirMapAdvisoryManager::setROI(const QGCGeoBoundingCube& roi, bool reset)
{
    //-- If first time or we've moved more than ADVISORY_UPDATE_DISTANCE, ask for updates.
    if(reset || (!_lastROI.isValid() || _lastROI.pointNW.distanceTo(roi.pointNW) > ADVISORY_UPDATE_DISTANCE || _lastROI.pointSE.distanceTo(roi.pointSE) > ADVISORY_UPDATE_DISTANCE)) {
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
    return static_cast<int>(aa->color()) > static_cast<int>(bb->color());
}

//-----------------------------------------------------------------------------
void
AirMapAdvisoryManager::_requestAdvisories()
{
    qCDebug(AirMapManagerLog) << "Advisories Request (ROI Changed)";
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Advisories";
        _valid = false;
        emit advisoryChanged();
        return;
    }
    _valid = false;
    _advisories.clearAndDeleteContents();
    Advisory::Search::Parameters params;
    //-- Geometry
    Geometry::Polygon polygon;
    //-- Get ROI bounding box, clipping to max area of interest
    for (const auto& qcoord : _lastROI.polygon2D(qgcApp()->toolbox()->airspaceManager()->maxAreaOfInterest())) {
        Geometry::Coordinate coord;
        coord.latitude  = qcoord.latitude();
        coord.longitude = qcoord.longitude();
        polygon.outer_ring.coordinates.push_back(coord);
    }
    params.geometry = Geometry(polygon);
    //-- Rulesets
    auto* pRulesMgr = qobject_cast<AirMapRulesetsManager*>(qgcApp()->toolbox()->airspaceManager()->ruleSets());
    QString ruleIDs;
    if(pRulesMgr) {
        for(int rs = 0; rs < pRulesMgr->ruleSets()->count(); rs++) {
            AirMapRuleSet* ruleSet = qobject_cast<AirMapRuleSet*>(pRulesMgr->ruleSets()->get(rs));
            //-- If this ruleset is selected
            if(ruleSet && ruleSet->selected()) {
                ruleIDs = ruleIDs + ruleSet->id();
                //-- Separate rules with commas
                if(rs < pRulesMgr->ruleSets()->count() - 1) {
                    ruleIDs = ruleIDs + ",";
                }
            }
        }
    }
    if(ruleIDs.isEmpty()) {
        qCDebug(AirMapManagerLog) << "No rules defined. Not updating Advisories";
        _valid = false;
        emit advisoryChanged();
        return;
    }
    params.rulesets = ruleIDs.toStdString();
    //-- Time
    quint64 start   = static_cast<quint64>(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    quint64 end     = start + 60 * 30 * 1000;
    params.start    = airmap::from_milliseconds_since_epoch(airmap::milliseconds(static_cast<qint64>(start)));
    params.end      = airmap::from_milliseconds_since_epoch(airmap::milliseconds(static_cast<qint64>(end)));
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->advisory().search(params, [this, isAlive](const Advisory::Search::Result& result) {
        if (!isAlive.lock()) return;
        if (result) {
            qCDebug(AirMapManagerLog) << "Successful advisory search. Items:" << result.value().size();
            _airspaceColor = AirspaceAdvisoryProvider::Green;
            for (const auto& advisory : result.value()) {
                AirMapAdvisory* pAdvisory = new AirMapAdvisory(this);
                pAdvisory->_id          = QString::fromStdString(advisory.advisory.airspace.id());
                pAdvisory->_name        = QString::fromStdString(advisory.advisory.airspace.name());
                pAdvisory->_type        = static_cast<AirspaceAdvisory::AdvisoryType>(advisory.advisory.airspace.type());
                pAdvisory->_color       = static_cast<AirspaceAdvisoryProvider::AdvisoryColor>(advisory.color);
                if(pAdvisory->_color > _airspaceColor) {
                    _airspaceColor = pAdvisory->_color;
                }
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

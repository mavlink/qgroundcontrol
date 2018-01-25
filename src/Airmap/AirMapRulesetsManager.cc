/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirMapRulesetsManager.h"

//-----------------------------------------------------------------------------
AirMapRulesetsManager::AirMapRulesetsManager(AirMapSharedState& shared)
    : _shared(shared)
{
}

//-----------------------------------------------------------------------------
void AirMapRulesetsManager::setROI(const QGeoCoordinate& center)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Airspace";
        return;
    }
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapRestrictionManager::updateROI: state not idle";
        return;
    }
    qCDebug(AirMapManagerLog) << "Setting ROI for Rulesets";
    _state = State::RetrieveItems;
    RuleSets::Search::Parameters params;
    params.geometry = Geometry::point(center.latitude(), center.longitude());
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->rulesets().search(params,
            [this, isAlive](const RuleSets::Search::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::RetrieveItems) return;
        if (result) {
            const std::vector<RuleSet>& rulesets = result.value();
            qCDebug(AirMapManagerLog)<<"Successful rulesets search. Items:" << rulesets.size();
            for (const auto& ruleset : rulesets) {
                qDebug() << "------------------------------------------";
                qDebug() << "Jurisdiction:" << ruleset.jurisdiction.name.data() << (int)ruleset.jurisdiction.region;
                qDebug() << "Name:        " << ruleset.name.data();
                qDebug() << "Short Name:  " << ruleset.short_name.data();
                qDebug() << "Description: " << ruleset.description.data();
                qDebug() << "Is default:  " << ruleset.is_default;
                qDebug() << "Applicable to these airspace types:";
                for (const auto& airspaceType : ruleset.airspace_types) {
                    qDebug() << airspaceType.data();
                }
                qDebug() << "Rules:";
                for (const auto& rule : ruleset.rules) {
                    qDebug() << "    --------------------------------------";
                    qDebug() << "    " << rule.short_text.data();
                    qDebug() << "    " << rule.description.data();
                    qDebug() << "    " << rule.display_order;
                    qDebug() << "    " << (int)rule.status;
                    qDebug() << "     Features:";
                    for (const auto& feature : rule.features) {
                        qDebug() << "        " << feature.name.data();
                        qDebug() << "        " << feature.description.data();
                        qDebug() << "        " << (int)feature.status;
                    }
                }
            }
        } else {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to retrieve RuleSets",
                    QString::fromStdString(result.error().message()), description);
        }
        emit requestDone(true);
        _state = State::Idle;
    });
}

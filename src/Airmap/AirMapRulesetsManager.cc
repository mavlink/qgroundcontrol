/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapRulesetsManager.h"
#include "AirMapManager.h"

using namespace airmap;

//-----------------------------------------------------------------------------
AirMapRuleFeature::AirMapRuleFeature(QObject* parent)
    : AirspaceRuleFeature(parent)
{
}

//-----------------------------------------------------------------------------
AirMapRuleFeature::AirMapRuleFeature(airmap::RuleSet::Feature feature, QObject* parent)
    : AirspaceRuleFeature(parent)
    , _feature(feature)
{
    //-- TODO: Read possible saved value from previous runs
}

//-----------------------------------------------------------------------------
AirspaceRuleFeature::Type
AirMapRuleFeature::type()
{
    return AirspaceRuleFeature::Unknown;
}

//-----------------------------------------------------------------------------
AirspaceRuleFeature::Unit
AirMapRuleFeature::unit()
{
    return AirspaceRuleFeature::UnknownUnit;
}

//-----------------------------------------------------------------------------
AirspaceRuleFeature::Measurement
AirMapRuleFeature::measurement()
{
    return AirspaceRuleFeature::UnknownMeasurement;
}

//-----------------------------------------------------------------------------
AirMapRule::AirMapRule(QObject* parent)
    : AirspaceRule(parent)
{
}

//-----------------------------------------------------------------------------
AirMapRule::AirMapRule(const airmap::RuleSet::Rule& rule, QObject* parent)
    : AirspaceRule(parent)
    , _rule(rule)
{
}

//-----------------------------------------------------------------------------
AirMapRule::~AirMapRule()
{
    _features.deleteListAndContents();
}

//-----------------------------------------------------------------------------
AirspaceRule::Status
AirMapRule::status()
{
    switch(_rule.status) {
    case RuleSet::Rule::Status::conflicting:
        return AirspaceRule::Conflicting;
    case RuleSet::Rule::Status::not_conflicting:
        return AirspaceRule::NotConflicting;
    case RuleSet::Rule::Status::missing_info:
        return AirspaceRule::MissingInfo;
    case RuleSet::Rule::Status::unknown:
    default:
        return AirspaceRule::Unknown;
    }
}

//-----------------------------------------------------------------------------
AirMapRuleSet::AirMapRuleSet(QObject* parent)
    : AirspaceRuleSet(parent)
    , _isDefault(false)
    , _selected(false)
    , _selectionType(AirspaceRuleSet::Optional)
{
}

//-----------------------------------------------------------------------------
AirMapRuleSet::~AirMapRuleSet()
{
    _rules.deleteListAndContents();
}

//-----------------------------------------------------------------------------
AirMapRulesetsManager::AirMapRulesetsManager(AirMapSharedState& shared)
    : _shared(shared)
{
}

//-----------------------------------------------------------------------------
static bool
rules_sort(QObject* a, QObject* b)
{
    AirMapRule* aa = qobject_cast<AirMapRule*>(a);
    AirMapRule* bb = qobject_cast<AirMapRule*>(b);
    if(!aa || !bb) return false;
    return (int)aa->order() > (int)bb->order();
}

//-----------------------------------------------------------------------------
void AirMapRulesetsManager::setROI(const QGeoCoordinate& center)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Airspace";
        return;
    }
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapRulesetsManager::updateROI: state not idle";
        return;
    }
    qCDebug(AirMapManagerLog) << "Setting ROI for Rulesets";
    _valid = false;
    _ruleSets.clearAndDeleteContents();
    _state = State::RetrieveItems;
    RuleSets::Search::Parameters params;
    params.geometry = Geometry::point(center.latitude(), center.longitude());
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->rulesets().search(params,
            [this, isAlive](const RuleSets::Search::Result& result) {
        if (!isAlive.lock()) return;
        if (_state != State::RetrieveItems) return;
        if (result) {
            const std::vector<RuleSet> rulesets = result.value();
            qCDebug(AirMapManagerLog) << "Successful rulesets search. Items:" << rulesets.size();
            for (const auto& ruleset : rulesets) {
                AirMapRuleSet* pRuleSet = new AirMapRuleSet(this);
                connect(pRuleSet, &AirspaceRuleSet::selectedChanged, this, &AirMapRulesetsManager::_selectedChanged);
                pRuleSet->_id          = QString::fromStdString(ruleset.id);
                pRuleSet->_name        = QString::fromStdString(ruleset.name);
                pRuleSet->_shortName   = QString::fromStdString(ruleset.short_name);
                pRuleSet->_description = QString::fromStdString(ruleset.description);
                pRuleSet->_isDefault   = ruleset.is_default;
                //-- TODO: This should be persistent and if the new incoming set has the
                //   same, previosuly selected rulesets, it should "remember".
                if(pRuleSet->_isDefault) {
                    pRuleSet->_selected = true;
                }
                switch(ruleset.selection_type) {
                case RuleSet::SelectionType::pickone:
                    pRuleSet->_selectionType = AirspaceRuleSet::Pickone;
                    break;
                case RuleSet::SelectionType::required:
                    pRuleSet->_selectionType = AirspaceRuleSet::Required;
                    pRuleSet->_selected = true;
                    break;
                default:
                case RuleSet::SelectionType::optional:
                    pRuleSet->_selectionType = AirspaceRuleSet::Optional;
                    break;
                }
                //-- Iterate Rules
                for (const auto& rule : ruleset.rules) {
                    AirMapRule* pRule = new AirMapRule(rule, this);
                    //-- Iterate Rule Features

                    //-- TODO: Rule features don't make sense as they are

                    pRuleSet->_rules.append(pRule);
                }
                //-- Sort rules by display order
                std::sort(pRuleSet->_rules.objectList()->begin(), pRuleSet->_rules.objectList()->end(), rules_sort);
                _ruleSets.append(pRuleSet);
                qCDebug(AirMapManagerLog) << "Adding ruleset" << pRuleSet->name();
                /*
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
                */
            }
            _valid = true;
        } else {
            QString description = QString::fromStdString(result.error().description() ? result.error().description().get() : "");
            emit error("Failed to retrieve RuleSets", QString::fromStdString(result.error().message()), description);
        }
        _state = State::Idle;
        emit ruleSetsChanged();
        emit selectedRuleSetsChanged();
    });
}

//-----------------------------------------------------------------------------
QString
AirMapRulesetsManager::selectedRuleSets()
{
    QString selection;
    for(int i = 0; i < _ruleSets.count(); i++) {
        AirMapRuleSet* rule = qobject_cast<AirMapRuleSet*>(_ruleSets.get(i));
        if(rule && rule->selected()) {
            selection += rule->shortName() + ", ";
        }
    }
    int idx = selection.lastIndexOf(", ");
    if(idx >= 0) {
        selection = selection.left(idx);
    }
    return selection;
}

//-----------------------------------------------------------------------------
void
AirMapRulesetsManager::_selectedChanged()
{
    emit selectedRuleSetsChanged();
    //-- TODO: Do whatever it is you do to select a rule
}

//-----------------------------------------------------------------------------
QStringList
AirMapRulesetsManager::rulesetsIDs()
{
    QStringList list;
    for(int i = 0; i < _ruleSets.count(); i++) {
        AirMapRuleSet* rule = qobject_cast<AirMapRuleSet*>(_ruleSets.get(i));
        if(rule && rule->selected()) {
            list << rule->id();
        }
    }
    return list;
}

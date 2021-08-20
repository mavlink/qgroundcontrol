/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceFlightPlanProvider.h"
#include "AirMapRulesetsManager.h"
#include "AirMapManager.h"
#include "QGCApplication.h"
#include <QSettings>

using namespace airmap;

static const char* kAirMapFeatureGroup = "AirMapFeatureGroup";

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
    //-- Restore persisted value (if it exists)
    QSettings settings;
    settings.beginGroup(kAirMapFeatureGroup);
    switch(_feature.type) {
    case RuleSet::Feature::Type::boolean:
        //-- For boolean, we have 3 states: 0 - false, 1 - true and 2 - not set
        _value = settings.value(name(), 2);
        break;;
    case RuleSet::Feature::Type::floating_point:
        _value = settings.value(name(), NAN);
        break;;
    case RuleSet::Feature::Type::string:
        _value = settings.value(name(), QString());
        break;;
    default:
        break;
    }
    settings.endGroup();
}

//-----------------------------------------------------------------------------
QVariant
AirMapRuleFeature::value()
{
    //qCDebug(AirMapManagerLog) << "Value of" << name() << "==>" << _value << type();
    return _value;
}

//-----------------------------------------------------------------------------
AirspaceRuleFeature::Type
AirMapRuleFeature::type()
{
    switch(_feature.type) {
    case RuleSet::Feature::Type::boolean:
        return AirspaceRuleFeature::Boolean;
    case RuleSet::Feature::Type::floating_point:
        return AirspaceRuleFeature::Float;
    case RuleSet::Feature::Type::string:
        return AirspaceRuleFeature::String;
    default:
        break;
    }
    return AirspaceRuleFeature::Unknown;
}

//-----------------------------------------------------------------------------
AirspaceRuleFeature::Unit
AirMapRuleFeature::unit()
{
    switch(_feature.unit) {
    case RuleSet::Feature::Unit::kilograms:
        return AirspaceRuleFeature::Kilogram;
    case RuleSet::Feature::Unit::meters:
        return AirspaceRuleFeature::Meters;
    case RuleSet::Feature::Unit::meters_per_sec:
        return AirspaceRuleFeature::MetersPerSecond;
    default:
        break;
    }
    return AirspaceRuleFeature::UnknownUnit;
}

//-----------------------------------------------------------------------------
AirspaceRuleFeature::Measurement
AirMapRuleFeature::measurement()
{
    switch(_feature.measurement) {
    case RuleSet::Feature::Measurement::speed:
        return AirspaceRuleFeature::Speed;
    case RuleSet::Feature::Measurement::weight:
        return AirspaceRuleFeature::Weight;
    case RuleSet::Feature::Measurement::distance:
        return AirspaceRuleFeature::Distance;
    default:
        break;
    }
    return AirspaceRuleFeature::UnknownMeasurement;
}

//-----------------------------------------------------------------------------
void
AirMapRuleFeature::setValue(const QVariant val)
{
    switch(_feature.type) {
    case RuleSet::Feature::Type::boolean:
        if(val.toInt() != 0 && val.toInt() != 1) {
            return;
        }
        break;
    case RuleSet::Feature::Type::floating_point:
        if(!std::isfinite(val.toDouble())) {
            return;
        }
        break;;
    case RuleSet::Feature::Type::string:
        if(val.toString().isEmpty()) {
            return;
        }
        break;;
    default:
        return;
    }
    _value = val;
    //-- Make value persistent
    QSettings settings;
    settings.beginGroup(kAirMapFeatureGroup);
    settings.setValue(name(), _value);
    settings.endGroup();
    emit valueChanged();
    qgcApp()->toolbox()->airspaceManager()->flightPlan()->setDirty(true);
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
void
AirMapRuleSet::setSelected(bool sel)
{
    if(_selectionType != AirspaceRuleSet::Required) {
        if(_selected != sel) {
            _selected = sel;
            emit selectedChanged();
        }
    } else {
        if(!_selected) {
            _selected = true;
            emit selectedChanged();
        }
    }
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
    return static_cast<int>(aa->order()) > static_cast<int>(bb->order());
}

//-----------------------------------------------------------------------------
void AirMapRulesetsManager::setROI(const QGCGeoBoundingCube& roi, bool reset)
{
    Q_UNUSED(reset);
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Not updating Airspace";
        return;
    }
    if (_state != State::Idle) {
        qCWarning(AirMapManagerLog) << "AirMapRulesetsManager::updateROI: state not idle";
        return;
    }
    qCDebug(AirMapManagerLog) << "Rulesets Request (ROI Changed)";
    _valid = false;
    //-- Save current selection state
    QMap<QString, bool> selectionSet;
    for(int rs = 0; rs < ruleSets()->count(); rs++) {
        AirMapRuleSet* ruleSet = qobject_cast<AirMapRuleSet*>(ruleSets()->get(rs));
        selectionSet[ruleSet->id()] = ruleSet->selected();
    }
    _ruleSets.clearAndDeleteContents();
    _state = State::RetrieveItems;
    RuleSets::Search::Parameters params;
    params.authorization = _shared.loginToken().toStdString();
    //-- Geometry: Polygon
    Geometry::Polygon polygon;
    //-- Get ROI bounding box, clipping to max area of interest
    for (const auto& qcoord : roi.polygon2D(qgcApp()->toolbox()->airspaceManager()->maxAreaOfInterest())) {
        Geometry::Coordinate coord;
        coord.latitude  = qcoord.latitude();
        coord.longitude = qcoord.longitude();
        polygon.outer_ring.coordinates.push_back(coord);
    }
    params.geometry = Geometry(polygon);
    std::weak_ptr<LifetimeChecker> isAlive(_instance);
    _shared.client()->rulesets().search(params,
            [this, isAlive, selectionSet](const RuleSets::Search::Result& result) {
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
                //-- Restore selection set (if any)
                if(selectionSet.contains(pRuleSet->id())) {
                    pRuleSet->_selected = selectionSet[pRuleSet->id()];
                } else {
                    if(pRuleSet->_isDefault) {
                        pRuleSet->_selected = true;
                    }
                }
                switch(ruleset.selection_type) {
                case RuleSet::SelectionType::pickone:
                    pRuleSet->_selectionType = AirspaceRuleSet::Pickone;
                    break;
                case RuleSet::SelectionType::required:
                    pRuleSet->_selectionType = AirspaceRuleSet::Required;
                    pRuleSet->_selected = true;
                    break;
                case RuleSet::SelectionType::optional:
                    pRuleSet->_selectionType = AirspaceRuleSet::Optional;
                    break;
                }
                //-- Iterate Rules
                for (const auto& rule : ruleset.rules) {
                    AirMapRule* pRule = new AirMapRule(rule, this);
                    //-- Iterate Rule Features
                    for (const auto& feature : rule.features) {
                        AirMapRuleFeature* pFeature = new AirMapRuleFeature(feature, this);
                        pRule->_features.append(pFeature);
                    }
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
                    qDebug() << "  " << airspaceType.data();
                }
                qDebug() << "Rules:";
                for (const auto& rule : ruleset.rules) {
                    qDebug() << "    --------------------------------------";
                    qDebug() << "    short_text:   " << rule.short_text.data();
                    qDebug() << "    description:  " << rule.description.data();
                    qDebug() << "    display_order:" << rule.display_order;
                    qDebug() << "    status:       " << (int)rule.status;
                    qDebug() << "            ------------------------------";
                    qDebug() << "            Features:";
                    for (const auto& feature : rule.features) {
                        qDebug() << "            name:       " << feature.name.data();
                        qDebug() << "            description:" << feature.description.data();
                        qDebug() << "            status:     " << (int)feature.status;
                        qDebug() << "            type:       " << (int)feature.type;
                        qDebug() << "            measurement:" << (int)feature.measurement;
                        qDebug() << "            unit:       " << (int)feature.unit;
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
void
AirMapRulesetsManager::clearAllFeatures()
{
    QSettings settings;
    settings.beginGroup(kAirMapFeatureGroup);
    settings.remove("");
    settings.endGroup();
}


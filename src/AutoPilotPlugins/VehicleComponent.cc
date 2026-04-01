#include "VehicleComponent.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>

QGC_LOGGING_CATEGORY(VehicleComponentLog, "AutoPilotPlugins.VehicleComponent");

VehicleComponent::VehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent, QObject *parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _autopilot(autopilot)
    , _KnownVehicleComponent(KnownVehicleComponent)
{
    // qCDebug(VehicleComponentLog) << Q_FUNC_INFO << this;

    if (!vehicle || !autopilot) {
        qCWarning(VehicleComponentLog) << "Internal error";
    }
}

VehicleComponent::~VehicleComponent()
{
    // qCDebug(VehicleComponentLog) << Q_FUNC_INFO << this;
}

QStringList VehicleComponent::sections() const
{
    _ensureSectionsCached();

    if (_repeatFilters.isEmpty()) {
        return _expandedSections;
    }

    auto *pm = _vehicle ? _vehicle->parameterManager() : nullptr;
    if (!pm) {
        return _expandedSections;
    }

    QStringList result = _expandedSections;
    for (const auto &filter : _repeatFilters) {
        bool hasDisabled = false;
        for (int i = 0; i < filter.sectionNames.size(); i++) {
            if (!pm->parameterExists(ParameterManager::defaultComponentId, filter.paramNames[i]))
                continue;
            if (pm->getParameter(ParameterManager::defaultComponentId, filter.paramNames[i])->rawValue().toInt() == filter.disabledValue) {
                result.removeAll(filter.sectionNames[i]);
                hasDisabled = true;
            }
        }
        if (hasDisabled && !filter.disabledHeading.isEmpty()) {
            result.append(filter.disabledHeading);
        }
    }

    return result;
}

QVariantMap VehicleComponent::sectionKeywords() const
{
    _ensureSectionsCached();
    QVariantMap map;
    for (auto it = _sectionKeywords.constBegin(); it != _sectionKeywords.constEnd(); ++it) {
        map.insert(it.key(), it.value());
    }
    return map;
}

void VehicleComponent::_ensureSectionsCached() const
{
    if (_sectionsCached) {
        return;
    }
    _sectionsCached = true;

    const QString path = vehicleConfigJson();
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(VehicleComponentLog) << "Failed to open page definition:" << path;
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(VehicleComponentLog) << "Failed to parse page definition:" << path << parseError.errorString();
        return;
    }
    const QJsonObject root = doc.object();
    const QJsonObject constants = root.value("constants").toObject();
    const QJsonArray sectionsArray = root.value("sections").toArray();

    auto resolveConstantInt = [&](const QString &ref) -> int {
        if (constants.contains(ref)) {
            const QJsonValue v = constants.value(ref);
            return v.isDouble() ? v.toInt() : v.toString().toInt();
        }
        // disabledParamValue must reference a constant or be a numeric literal
        return ref.toInt();
    };

    for (const QJsonValue &val : sectionsArray) {
        const QJsonObject secObj = val.toObject();
        const QString name = secObj.value("title").toString();
        if (name.isEmpty()) {
            continue;
        }

        // Collect translatable search terms: title + explicit keywords + control labels
        // Stored in original case so QML can pass them through qsTranslate() at search time
        QStringList terms;
        terms.append(name);
        const QJsonArray kwArray = secObj.value("keywords").toArray();
        for (const QJsonValue &kw : kwArray) {
            terms.append(kw.toString());
        }
        for (const QJsonValue &ctrl : secObj.value("controls").toArray()) {
            const QString label = ctrl.toObject().value("label").toString();
            if (!label.isEmpty()) {
                terms.append(label);
            }
        }
        terms.removeDuplicates();

        const QJsonObject repeatObj = secObj.value("repeat").toObject();
        if (!repeatObj.isEmpty() && _vehicle && _vehicle->parameterManager()) {
            const QString paramPrefix = repeatObj.value("paramPrefix").toString();
            const QString probePostfix = repeatObj.value("probePostfix").toString();
            const QString indexing = repeatObj.value("indexing").toString();
            const int startIndex = repeatObj.value("startIndex").toInt(1);
            const bool firstOmits = repeatObj.value("firstIndexOmitsNumber").toBool(false);

            // Check for enableParam filtering
            const QString enableParam = repeatObj.value("enableParam").toString();
            const QString disabledParamValueRef = repeatObj.value("disabledParamValue").toString();
            const QJsonObject disabledSectionObj = repeatObj.value("disabledSection").toObject();
            const bool hasFilter = !enableParam.isEmpty() && !disabledParamValueRef.isEmpty();

            RepeatFilter filter;
            if (hasFilter) {
                filter.disabledValue = resolveConstantInt(disabledParamValueRef);
                filter.disabledHeading = disabledSectionObj.value("heading").toString();
            }

            int count = 0;
            if (indexing == QStringLiteral("apm_battery")) {
                auto battPrefix = [&](int i) -> QString {
                    if (i == 0) return paramPrefix + QStringLiteral("_");
                    if (i <= 8) return paramPrefix + QString::number(i + 1) + QStringLiteral("_");
                    return paramPrefix + QChar('A' + i - 9) + QStringLiteral("_");
                };
                auto battLabel = [](int i) -> QString {
                    if (i <= 8) return QString::number(i + 1);
                    return QString(QChar('A' + i - 9));
                };
                for (int i = 0; i < 16; i++) {
                    const QString probeParam = battPrefix(i) + probePostfix;
                    if (!_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, probeParam))
                        break;
                    count++;
                }
                if (count <= 1) {
                    _expandedSections.append(name);
                    _sectionKeywords.insert(name, terms);
                    if (hasFilter) {
                        filter.sectionNames.append(name);
                        filter.paramNames.append(battPrefix(0) + enableParam);
                    }
                } else {
                    for (int i = 0; i < count; i++) {
                        const QString sectionName = name + QStringLiteral(" ") + battLabel(i);
                        _expandedSections.append(sectionName);
                        _sectionKeywords.insert(sectionName, terms);
                        if (hasFilter) {
                            filter.sectionNames.append(sectionName);
                            filter.paramNames.append(battPrefix(i) + enableParam);
                        }
                    }
                }
            } else {
                for (int i = startIndex; ; i++) {
                    const QString idx = (firstOmits && i == startIndex) ? QString() : QString::number(i);
                    const QString probeParam = paramPrefix + idx + probePostfix;
                    if (!_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, probeParam))
                        break;
                    count++;
                }

                if (count <= 1) {
                    _expandedSections.append(name);
                    _sectionKeywords.insert(name, terms);
                    if (hasFilter) {
                        const QString idx = (firstOmits) ? QString() : QString::number(startIndex);
                        filter.sectionNames.append(name);
                        filter.paramNames.append(paramPrefix + idx + enableParam);
                    }
                } else {
                    for (int i = 0; i < count; i++) {
                        const QString idx = (firstOmits && i == 0) ? QString() : QString::number(startIndex + i);
                        const QString sectionName = name + QStringLiteral(" ") + QString::number(startIndex + i);
                        _expandedSections.append(sectionName);
                        _sectionKeywords.insert(sectionName, terms);
                        if (hasFilter) {
                            filter.sectionNames.append(sectionName);
                            filter.paramNames.append(paramPrefix + idx + enableParam);
                        }
                    }
                }
            }

            if (hasFilter) {
                _repeatFilters.append(filter);
            }
        } else {
            if (!_expandedSections.contains(name)) {
                _expandedSections.append(name);
            }
            _sectionKeywords.insert(name, terms);
        }
    }
}

void VehicleComponent::addSummaryQmlComponent(QQmlContext *context, QQuickItem *parent)
{
    if (!context) {
        qCWarning(VehicleComponentLog) << "Internal error";
        return;
    }

    QQmlComponent component = new QQmlComponent(context->engine(), QUrl::fromUserInput("qrc:/qml/VehicleComponentSummaryButton.qml"), this);
    if (component.status() == QQmlComponent::Error) {
        qCWarning(VehicleComponentLog) << component.errors();
        return;
    }

    QQuickItem *const item = qobject_cast<QQuickItem*>(component.create(context));
    if (!item) {
        qCWarning(VehicleComponentLog) << "Internal error";
        return;
    }

    item->setParentItem(parent);
    item->setProperty("vehicleComponent", QVariant::fromValue(this));
}

void VehicleComponent::setupTriggerSignals()
{
    // Watch for changed on trigger list params
    for (const QString &paramName: setupCompleteChangedTriggerList()) {
        if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, paramName)) {
            Fact *const fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, paramName);
            (void) connect(fact, &Fact::valueChanged, this, &VehicleComponent::_triggerUpdated);
        }
    }

    // Watch enableParam facts so the sections list updates when items are enabled/disabled
    _ensureSectionsCached();
    for (const auto &filter : _repeatFilters) {
        for (const QString &paramName : filter.paramNames) {
            if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, paramName)) {
                Fact *const fact = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, paramName);
                (void) connect(fact, &Fact::valueChanged, this, &VehicleComponent::sectionsChanged);
            }
        }
    }
}

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtQmlIntegration/QtQmlIntegration>

#include "AutoPilotPlugin.h"

class Vehicle;
class QQuickItem;
class QQmlContext;

Q_DECLARE_LOGGING_CATEGORY(VehicleComponentLog)

/// A vehicle component is an object which abstracts the physical portion of a vehicle into a set of
/// configurable values and user interface.
class VehicleComponent : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString  name                                                READ name                   CONSTANT)
    Q_PROPERTY(QString  description                                         READ description            CONSTANT)
    Q_PROPERTY(bool     requiresSetup                                       READ requiresSetup          CONSTANT)
    Q_PROPERTY(bool     setupComplete                                       READ setupComplete          STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(QString  iconResource                                        READ iconResource           CONSTANT)
    Q_PROPERTY(QUrl     setupSource                                         READ setupSource            NOTIFY setupSourceChanged)
    Q_PROPERTY(QUrl     summaryQmlSource                                    READ summaryQmlSource       CONSTANT)
    Q_PROPERTY(bool     allowSetupWhileArmed                                READ allowSetupWhileArmed   CONSTANT)
    Q_PROPERTY(bool     allowSetupWhileFlying                               READ allowSetupWhileFlying  CONSTANT)
    Q_PROPERTY(AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent READ KnownVehicleComponent  CONSTANT)
    Q_PROPERTY(QStringList sections                                         READ sections               NOTIFY sectionsChanged)
    Q_PROPERTY(QVariantMap sectionKeywords                                   READ sectionKeywords        NOTIFY sectionsChanged)
    Q_PROPERTY(QString  vehicleConfigJson                                   READ vehicleConfigJson      CONSTANT)
    Q_PROPERTY(bool     showFirstSectionOnRootClick                         READ showFirstSectionOnRootClick CONSTANT)

public:
    explicit VehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent, QObject *parent = nullptr);
    virtual ~VehicleComponent();

    virtual QString name() const = 0;
    virtual QString description() const { return QString(); }
    virtual QString iconResource() const = 0;
    virtual bool requiresSetup() const = 0;
    virtual bool setupComplete() const = 0;
    virtual QUrl setupSource() const = 0;
    virtual QUrl summaryQmlSource() const = 0;

    /// Resource path to a VehicleConfig.json page definition, or empty if none.
    virtual QString vehicleConfigJson() const { return QString(); }

    /// Section names for sidebar navigation. Auto-populated from vehicleConfigJson() JSON.
    /// Repeat sections are expanded by probing vehicle parameters.
    virtual QStringList sections() const;

    /// Search keywords per section, keyed by section title. Values are original-case translatable terms.
    QVariantMap sectionKeywords() const;

    /// When true, clicking the root component in the tree selects the first section instead of showing all.
    virtual bool showFirstSectionOnRootClick() const { return false; }

    /// Returns setup-complete status for a named section. Default returns true (no per-section tracking).
    Q_INVOKABLE virtual bool sectionSetupComplete(const QString &sectionName) const { Q_UNUSED(sectionName); return true; }

    // @return true: Setup panel can be shown while vehicle is armed
    virtual bool allowSetupWhileArmed() const { return false; }

    // @return true: Setup panel can be shown while vehicle is flying (and armed)
    virtual bool allowSetupWhileFlying() const { return false; }

    virtual void addSummaryQmlComponent(QQmlContext* context, QQuickItem* parent);

    /// Returns an list of parameter names for which a change should cause the setupCompleteChanged
    /// signal to be emitted.
    virtual QStringList setupCompleteChangedTriggerList() const = 0;

    /// Should be called after the component is created (but not in constructor) to setup the
    /// signals which are used to track parameter changes which affect setupComplete state.
    virtual void setupTriggerSignals();

    AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent() const { return _KnownVehicleComponent; }

signals:
    void setupCompleteChanged();
    void setupSourceChanged();
    void sectionsChanged();

protected slots:
    void _triggerUpdated(QVariant /*value*/) { emit setupCompleteChanged(); }

protected:
    Vehicle *_vehicle = nullptr;
    AutoPilotPlugin *_autopilot = nullptr;
    AutoPilotPlugin::KnownVehicleComponent _KnownVehicleComponent;

private:
    /// Lazily parse the vehicleConfigJson() file to populate _expandedSections and _repeatFilters.
    void _ensureSectionsCached() const;

    /// Metadata for a repeat group that has enableParam/disabledParamValue filtering.
    struct RepeatFilter {
        QStringList sectionNames;   ///< Expanded section names in this repeat group
        QStringList paramNames;     ///< Corresponding full enableParam names (e.g., BATT_MONITOR)
        int         disabledValue = 0;
        QString     disabledHeading; ///< From disabledSection.heading (empty if no disabledSection)
    };

    mutable QStringList            _expandedSections;  ///< All sections before enable/disable filtering
    mutable QVector<RepeatFilter>  _repeatFilters;     ///< Filter metadata for repeat groups with enableParam
    mutable QMap<QString, QStringList> _sectionKeywords;  ///< section title -> original-case translatable search terms
    mutable bool                   _sectionsCached = false;
};

#pragma once

#include <limits>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtQmlIntegration/QtQmlIntegration>

#include "AutoPilotPlugin.h"
#include "VehicleSummaryItem.h"

class Fact;
class Vehicle;
class QQuickItem;
class QQmlContext;

/// \brief A vehicle component is an object which abstracts the physical portion of a vehicle into a set of
/// configurable values and user interface.

class VehicleComponent : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString  name                                                READ name                   CONSTANT)
    Q_PROPERTY(QString  description                                         READ description            CONSTANT)
    Q_PROPERTY(bool     requiresSetup                                       READ requiresSetup          NOTIFY requiresSetupChanged)
    Q_PROPERTY(int      setupOrder                                          READ setupOrder             CONSTANT)
    Q_PROPERTY(bool     setupComplete                                       READ setupComplete          STORED false NOTIFY setupCompleteChanged)
    Q_PROPERTY(QString  iconResource                                        READ iconResource           CONSTANT)
    Q_PROPERTY(QUrl     setupSource                                         READ setupSource            NOTIFY setupSourceChanged)
    Q_PROPERTY(QUrl     summaryQmlSource                                    READ summaryQmlSource       CONSTANT)
    Q_PROPERTY(QList<VehicleSummaryItem> summaryItems                       READ summaryItems           STORED false NOTIFY summaryItemsChanged)
    Q_PROPERTY(bool     allowSetupWhileArmed                                READ allowSetupWhileArmed   CONSTANT)
    Q_PROPERTY(bool     allowSetupWhileFlying                               READ allowSetupWhileFlying  CONSTANT)
    Q_PROPERTY(AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent READ KnownVehicleComponent  CONSTANT)
    Q_PROPERTY(QStringList sectionIds                                       READ sectionIds             NOTIFY sectionIdsChanged)
    Q_PROPERTY(QVariantMap sectionKeywords                                  READ sectionKeywords        NOTIFY sectionIdsChanged)
    Q_PROPERTY(QStringList searchKeywords READ searchKeywords CONSTANT)
    Q_PROPERTY(QString  vehicleConfigJson                                   READ vehicleConfigJson      CONSTANT)
    Q_PROPERTY(bool     showFirstSectionOnRootClick                         READ showFirstSectionOnRootClick CONSTANT)

public:
    explicit VehicleComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, AutoPilotPlugin::KnownVehicleComponent KnownVehicleComponent, QObject *parent = nullptr);
    virtual ~VehicleComponent();

    virtual QString name() const = 0;
    virtual QString description() const { return QString(); }
    virtual QString iconResource() const = 0;
    virtual bool requiresSetup() const = 0;

    /// Where the component comes in the recommended order of setting up a vehicle, lowest first. Components that
    /// do not set one come after those that do.
    virtual int setupOrder() const { return std::numeric_limits<int>::max(); }
    virtual bool setupComplete() const = 0;
    virtual QUrl setupSource() const = 0;
    virtual QUrl summaryQmlSource() const = 0;

    /// Values for this component's card on the Summary page. A component with none falls back to summaryQmlSource().
    virtual QList<VehicleSummaryItem> summaryItems() const { return {}; }

    /// Parameters whose change emits summaryItemsChanged, besides those that change setupComplete
    virtual QStringList summaryChangedTriggerList() const { return {}; }

    /// Resource path to a VehicleConfig.json page definition, or empty if none.
    virtual QString vehicleConfigJson() const { return QString(); }

    /// Untranslated section IDs for sidebar navigation. Auto-populated from vehicleConfigJson() JSON.
    /// Repeat sections are expanded by probing vehicle parameters. IDs are stable across locales and
    /// are used for section filtering; display uses sectionDisplayName() or the JSON translation context.
    virtual QStringList sectionIds() const;

    /// Translated display name for a section ID. Default returns the ID unchanged (JSON-driven
    /// components are translated by the view using the JSON filename context).
    Q_INVOKABLE virtual QString sectionDisplayName(const QString& sectionId) const { return sectionId; }

    /// Search keywords per section, keyed by section ID. Values are original-case translatable terms.
    QVariantMap sectionKeywords() const;

    /// Translated search terms for the whole component, for pages without vehicleConfigJson() sections.
    virtual QStringList searchKeywords() const { return QStringList(); }

    /// When true, clicking the root component in the tree selects the first section instead of showing all.
    virtual bool showFirstSectionOnRootClick() const { return false; }

    /// Returns setup-complete status for a section ID. Default returns true (no per-section tracking).
    Q_INVOKABLE virtual bool sectionSetupComplete(const QString &sectionId) const { Q_UNUSED(sectionId); return true; }

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
    void requiresSetupChanged();
    void setupCompleteChanged();
    void summaryItemsChanged();
    void setupSourceChanged();
    void sectionIdsChanged();

protected slots:
    void _triggerUpdated(QVariant /*value*/) { emit setupCompleteChanged(); }

protected:
    /// Fact of a parameter of the default component, nullptr when the vehicle does not have it
    Fact* _parameterFact(const QString& name) const;

    /// The fact's value followed by its units, when it has any
    static QString _valueWithUnits(const Fact* fact);

    Vehicle *_vehicle = nullptr;
    AutoPilotPlugin *_autopilot = nullptr;
    AutoPilotPlugin::KnownVehicleComponent _KnownVehicleComponent;

private:
    /// Lazily parse the vehicleConfigJson() file to populate _expandedSectionIds and _repeatFilters.
    void _ensureSectionsCached() const;

    /// Metadata for a repeat group that has enableParam/disabledParamValue filtering.
    struct RepeatFilter {
        QStringList sectionIds;     ///< Expanded section IDs in this repeat group
        QStringList paramNames;     ///< Corresponding full enableParam names (e.g., BATT_MONITOR)
        int         disabledValue = 0;
        QString     disabledHeading; ///< From disabledSection.heading (empty if no disabledSection)
    };

    mutable QStringList            _expandedSectionIds;  ///< All section IDs before enable/disable filtering
    mutable QVector<RepeatFilter>  _repeatFilters;     ///< Filter metadata for repeat groups with enableParam
    mutable QMap<QString, QStringList> _sectionKeywords;  ///< section ID -> original-case translatable search terms
    mutable bool                   _sectionsCached = false;
};

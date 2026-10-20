#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"
#include "PX4SerialPortConfig.h"

class Fact;
class ParameterManager;
class QmlObjectListModel;

/// One serial port row on the PX4 Ports page
class PX4SerialPortRow : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by PX4SerialPortsController")
    Q_MOC_INCLUDE("Fact.h")
    Q_PROPERTY(int portId READ portId CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    /// "None" first, then each function allowed on the port. "None" is left out while a function that can't be removed
    /// from its default port is on it.
    Q_PROPERTY(QStringList functionLabels READ functionLabels NOTIFY functionLabelsChanged)
    /// true: several functions can run on the port at once, so each one is turned on or off separately
    Q_PROPERTY(bool shared READ shared CONSTANT)
    /// Function on the port, as an index into functionLabels. The first one if there is a conflict.
    Q_PROPERTY(int functionIndex READ functionIndex NOTIFY functionIndexChanged)
    /// Parallel to functionLabels: true if that function is on the port. Used for shared ports.
    Q_PROPERTY(QVariantList functionEnabled READ functionEnabled NOTIFY functionEnabledChanged)
    Q_PROPERTY(QString conflictText READ conflictText NOTIFY conflictTextChanged)
    Q_PROPERTY(Fact* baudFact READ baudFact NOTIFY baudFactChanged)
    /// true: the baud rate parameter is being read from the vehicle
    Q_PROPERTY(bool baudLoading READ baudLoading NOTIFY baudLoadingChanged)
    Q_PROPERTY(QVariantList optionFacts READ optionFacts NOTIFY optionFactsChanged)
    /// true: some option parameters are being read from the vehicle
    Q_PROPERTY(bool optionsLoading READ optionsLoading NOTIFY optionsLoadingChanged)
    Q_PROPERTY(bool rebootRequired READ rebootRequired NOTIFY rebootRequiredChanged)

public:
    PX4SerialPortRow(int portId, const QString& label, const QStringList& functionParamNames,
                     const QStringList& functionLabels, QObject* parent = nullptr);

    int portId() const { return _portId; }

    QString label() const { return _label; }

    QStringList functionLabels() const { return _noneAvailable ? _functionLabels : _functionLabels.mid(1); }

    bool shared() const { return _shared; }

    int functionIndex() const { return _functionIndex; }

    QVariantList functionEnabled() const { return _functionEnabled; }

    QString conflictText() const { return _conflictText; }

    Fact* baudFact() const { return _baudFact; }

    bool baudLoading() const { return _baudLoading; }

    QVariantList optionFacts() const { return _optionFacts; }

    bool optionsLoading() const { return _optionsLoading; }

    bool rebootRequired() const { return _rebootRequired; }

    /// Function parameter for an index into functionLabels, empty for "None"
    QString functionParamName(int functionIndex) const;
    int functionIndexOf(const QString& functionParamName) const;

    void setNoneAvailable(bool noneAvailable);
    void setFunctionIndex(int functionIndex);
    void setFunctionEnabled(const QVariantList& functionEnabled);
    void setConflictText(const QString& conflictText);
    void setBaudFact(Fact* baudFact);
    void setBaudLoading(bool baudLoading);
    void setOptionFacts(const QVariantList& optionFacts);
    void setOptionsLoading(bool optionsLoading);
    void setRebootRequired(bool rebootRequired);

signals:
    void functionLabelsChanged();
    void functionIndexChanged();
    void functionEnabledChanged();
    void conflictTextChanged();
    void baudFactChanged();
    void baudLoadingChanged();
    void optionFactsChanged();
    void optionsLoadingChanged();
    void rebootRequiredChanged();

private:
    const int _portId;
    const QString _label;
    const QStringList _functionParamNames;  ///< Parallel to _functionLabels, empty for "None"
    const QStringList _functionLabels;      ///< Including "None", whether or not it is available
    bool _noneAvailable = true;
    const bool _shared;
    int _functionIndex = 0;
    QVariantList _functionEnabled;
    QString _conflictText;
    Fact* _baudFact = nullptr;
    bool _baudLoading = false;
    QVariantList _optionFacts;
    bool _optionsLoading = false;
    bool _rebootRequired = false;
};

/// Backs the PX4 Ports page: shows PX4's per-function serial port parameters as one row per port. Changes are
/// written immediately. All of them need a reboot, so the page shows that itself instead of the reboot notice.
class PX4SerialPortsController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel* ports READ ports CONSTANT)
    Q_PROPERTY(int changedPortCount READ changedPortCount NOTIFY changedPortCountChanged)

public:
    explicit PX4SerialPortsController(QObject* parent = nullptr);
    ~PX4SerialPortsController() override;

    QmlObjectListModel* ports() const { return _ports; }

    int changedPortCount() const { return _changedPortCount; }

    /// Makes the function at functionIndex (0 for none) the only function on the port. Ignored for shared ports, which
    /// use setPortFunctionEnabled.
    Q_INVOKABLE void setPortFunction(int portId, int functionIndex);

    /// Turns the function at functionIndex on or off on a shared port, leaving the port's other functions alone
    Q_INVOKABLE void setPortFunctionEnabled(int portId, int functionIndex, bool enabled);

    /// Writes back the values each parameter had when the page opened. Ignored while parameter writes are pending.
    Q_INVOKABLE void revertChanges();

signals:
    void changedPortCountChanged();

private slots:
    void _update();
    void _factAdded(int componentId, Fact* fact);
    void _parameterReadFailed(int componentId, const QString& paramName);

private:
    Fact* _fact(const QString& paramName);
    void _watchFact(Fact* fact);
    void _requestParameter(const QString& paramName);
    bool _differsFromInitial(const QString& paramName) const;
    void _writeParams(const QList<PX4SerialPortConfig::ParamWrite>& writes);
    PX4SerialPortRow* _row(int portId) const;

    QPointer<ParameterManager> _parameterManager;
    QmlObjectListModel* _ports = nullptr;
    QList<Fact*> _functionFacts;
    QHash<QString, QVariant> _initialValues;  ///< Value of each parameter the first time the page saw it
    QSet<QString> _requestedParams;           ///< Unsent parameters requested from the vehicle
    QSet<QString> _pendingParams;             ///< Requested parameters still waiting for a reply
    QSet<Fact*> _watchedFacts;
    QHash<QString, int> _optionParamPorts;    ///< Port whose row last showed each option, so a changed option stays
                                              ///< counted there after its function moves away
    int _changedPortCount = 0;
};

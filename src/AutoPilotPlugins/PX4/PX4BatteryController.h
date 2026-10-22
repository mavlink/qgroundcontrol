#pragma once

#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

class ParameterManager;

/// Controller for PX4BatteryComponent.qml. PX4 only streams the parameters a running module has looked up, such as a
/// power monitor driver's settings once the driver runs, so the page requests the rest by name.
class PX4BatteryController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QStringList pendingParams READ pendingParams NOTIFY pendingParamsChanged)
    Q_PROPERTY(int addedParamCount READ addedParamCount NOTIFY addedParamCountChanged)

public:
    explicit PX4BatteryController(QObject* parent = nullptr);

    /// Requests each named parameter the vehicle has not sent. A name stays in pendingParams until its value arrives
    /// or the read fails. Parameters the firmware does not have are dropped without telling the user, and are not
    /// requested again for the same vehicle.
    Q_INVOKABLE void requestParameters(const QStringList& names);

    /// True when the vehicle has the parameter and it is not waiting for its requested value
    Q_INVOKABLE bool parameterReady(const QString& name) const;

    QStringList pendingParams() const { return _pendingParams.values(); }

    /// Number of parameters the vehicle has added since the page opened, requested or not. Changes once the new
    /// parameter has its value and is ready, and before pendingParams signals the change, so bindings on it have
    /// updated by then. A failed read does not change it.
    int addedParamCount() const { return _addedParamCount; }

signals:
    void pendingParamsChanged();
    void addedParamCountChanged();

private slots:
    void _factAdded(int componentId, Fact* fact);
    void _parameterReadFailed(int componentId, const QString& paramName);

private:
    void _paramAdded(const QString& paramName);
    void _removePending(const QString& paramName);

    QPointer<ParameterManager> _parameterManager;
    QSet<QString> _requestedParams;
    QSet<QString> _pendingParams;
    int _addedParamCount = 0;
};

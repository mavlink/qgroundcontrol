/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

Q_DECLARE_LOGGING_CATEGORY(FactPanelControllerLog)

class AutoPilotPlugin;
class Vehicle;
class Fact;

/// Used for handling missing Facts from C++ code.
class FactPanelController : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("Fact.h")
    Q_PROPERTY(Vehicle *vehicle MEMBER _vehicle CONSTANT)

public:
    FactPanelController(QObject *parent = nullptr);
    ~FactPanelController();

    Q_INVOKABLE Fact *getParameterFact(int componentId, const QString &name, bool reportMissing = true) const;
    Q_INVOKABLE bool parameterExists(int componentId, const QString &name) const;

    /// Queries the vehicle for parameters which were not available on initial download but should be available now.
    /// Signals missingParametersAvailable when done. Only works for MAV_COMP_ID_AUTOPILOT1 parameters.
    Q_INVOKABLE void getMissingParameters(const QStringList &rgNames);

signals:
    void missingParametersAvailable();

protected:
    /// Checks for existence of the specified parameters
    ///     @return true: all parameters exists, false: parameters missing and reported
    bool _allParametersExists(int componentId, const QStringList &names) const;

    /// Report a missing parameter
    void _reportMissingParameter(int componentId, const QString &name) const;

    Vehicle *_vehicle = nullptr;
    AutoPilotPlugin *_autopilot = nullptr;

private slots:
    void _checkForMissingParameters();

private:
    QStringList _missingParameterWaitList;
    QTimer _missingParametersTimer;
};

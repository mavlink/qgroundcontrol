#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

class Vehicle;
class VehicleReadinessPX4Test;
class VehicleReadinessTest;

Q_DECLARE_LOGGING_CATEGORY(VehicleReadinessLog)

/// \brief Whether the vehicle would accept the Arm, Takeoff and Start Mission guided actions.
///
/// This is the single source of readiness for both the guided actions and the toolbar status. It is created by
/// FirmwarePlugin::createVehicleReadiness, since the firmware plugin decides which flight modes each guided action
/// arms and runs in. Readiness uses the best source available on the vehicle:
///   1. libevents health and arming checks (mode aware: arming checks while disarmed, mode
///      requirements while armed)
///   2. The SYS_STATUS prearm check bit (current mode only)
///   3. SYS_STATUS sensor health and vehicle setup, which only feeds the pre-flight status and never blocks actions
/// Readiness is a prediction. The vehicle has the final say on every command.
class VehicleReadiness : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool canArm READ canArm NOTIFY readinessChanged)
    Q_PROPERTY(bool canTakeoff READ canTakeoff NOTIFY readinessChanged)
    Q_PROPERTY(bool canStartMission READ canStartMission NOTIFY readinessChanged)
    Q_PROPERTY(Status status READ status NOTIFY readinessChanged)

public:
    explicit VehicleReadiness(Vehicle* vehicle);

    enum class Status
    {
        Unknown,  ///< Waiting for a health report the vehicle is expected to send, or the current mode is unknown to it
        NotReady,
        Ready,
    };
    Q_ENUM(Status)

    bool canArm() const { return _result.canArm; }

    bool canTakeoff() const { return _result.canTakeoff; }

    bool canStartMission() const { return _result.canStartMission; }

    Status status() const { return _result.status; }

    /// Re-evaluates readiness. Vehicle calls this once the firmware's subclass is fully constructed.
    void update();

signals:
    void readinessChanged();

protected:
    enum class GuidedAction
    {
        Takeoff,
        StartMission,
    };

    /// Flight modes a guided action uses, as the firmware plugin performs it
    struct ActionFlightModes
    {
        QString armFlightMode;     ///< Mode the vehicle is in when the action arms it
        QString targetFlightMode;  ///< Mode the action ends up in, which an armed vehicle switches into
    };

    /// Firmware returns the modes its plugin uses for the action. Unknown modes fall back to the current mode.
    virtual ActionFlightModes _actionFlightModes(GuidedAction /*action*/) const { return ActionFlightModes{}; }

    /// True if the firmware is known to send libevents health reports. Guided actions are then blocked until the
    /// first report arrives, unless initial connect completes without the vehicle supporting them.
    virtual bool _expectsHealthAndArmingChecks() const { return false; }

    /// Per action overrides of the generic readiness. Each is given the generic answer, which firmware can keep,
    /// tighten or replace to match how its plugin performs the action. An enforced pre-flight checklist still
    /// applies afterwards.
    /// @{
    virtual bool _canArm(bool genericCanArm) const { return genericCanArm; }

    virtual bool _canTakeoff(bool genericCanTakeoff) const { return genericCanTakeoff; }

    virtual bool _canStartMission(bool genericCanStartMission) const { return genericCanStartMission; }

    virtual Status _status(Status genericStatus) const { return genericStatus; }

    /// @}

    Vehicle* _vehicle = nullptr;

private:
    friend class VehicleReadinessPX4Test;
    friend class VehicleReadinessTest;

    /// libevents mode groups of an action's flight modes. -1 when unknown, which falls back to the current mode.
    struct ActionModeGroups
    {
        int armModeGroup = -1;
        int targetModeGroup = -1;
    };

    struct Inputs
    {
        bool armed = false;
        bool checklistBlocksArming = false;
        bool initialConnectComplete = false;
        bool healthReportWaitExpired = false;         ///< Stop waiting for a health report which has not arrived

        bool healthAndArmingChecksExpected = false;   ///< Firmware is known to send libevents health reports
        bool healthAndArmingChecksSupported = false;  ///< Vehicle metadata lists the libevents health protocol
        bool healthAndArmingChecksReported = false;   ///< A libevents health report has been received
        int currentModeGroup = -1;
        ActionModeGroups takeoff{};
        ActionModeGroups startMission{};
        quint64 canArmModeGroups = 0;
        quint64 canRunModeGroups = 0;

        bool prearmCheckAvailable = false;
        bool prearmCheckPassed = false;

        bool sensorsHealthy = false;
        bool setupComplete = false;
    };

    struct Result
    {
        bool canArm = false;
        bool canTakeoff = false;
        bool canStartMission = false;
        Status status = Status::Unknown;

        bool operator==(const Result& other) const = default;
    };

    static Result _evaluate(const Inputs& inputs);
    static void _applyChecklist(Result& result, const Inputs& inputs);
    Inputs _gatherInputs() const;
    ActionModeGroups _actionModeGroups(GuidedAction action) const;
    int _modeGroup(const QString& flightMode) const;

    void _healthReportWaitTimeout();

    /// How long after initial connect to wait for a health report before falling back to other sources
    static constexpr int _healthReportWaitMs = 10000;

    Result _result;
    QTimer _healthReportWaitTimer;
    bool _healthReportWaitExpired = false;
};

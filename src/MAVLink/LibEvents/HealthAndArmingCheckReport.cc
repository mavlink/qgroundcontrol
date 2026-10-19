#include "HealthAndArmingCheckReport.h"
#include "EventHandler.h"
#include "QmlObjectListModel.h"
#include "LibEvents.h"
#include "MAVLinkEnums.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(HealthAndArmingCheckReportLog, "MAVLink.LibEvents.HealthAndArmingCheckReport")

HealthAndArmingCheckProblem::HealthAndArmingCheckProblem(const QString& message, const QString& description, const QString& severity, QObject *parent)
    : QObject(parent)
    , _message(message)
    , _description(description)
    , _severity(severity)
{
    qCDebug(HealthAndArmingCheckReportLog) << this;
}

void HealthAndArmingCheckProblem::setAcknowledged(bool acknowledged)
{
    if (acknowledged != _acknowledged) {
        _acknowledged = acknowledged;
        emit acknowledgedChanged();
    }
}

/*===========================================================================*/

HealthAndArmingCheckReport::HealthAndArmingCheckReport(QObject *parent)
    : QObject(parent)
    , _problemsForCurrentMode(new QmlObjectListModel(this))
{
    qCDebug(HealthAndArmingCheckReportLog) << this;
}

HealthAndArmingCheckReport::~HealthAndArmingCheckReport()
{
    _problemsForCurrentMode->clearAndDeleteContents();
}

void HealthAndArmingCheckReport::update(uint8_t compid, const EventHandler &eventHandler, int flightModeGroup)
{
    if (compid != MAV_COMP_ID_AUTOPILOT1) {
        return;
    }

    // A mode missing from the metadata still gets the report: the mode group results are valid for other modes
    const bool currentModeKnown = (flightModeGroup >= 0) && (flightModeGroup < MaxModeGroups);
    if (!currentModeKnown) {
        flightModeGroup = -1;
        if (!_supported || (_modeGroupResults.currentModeGroup != -1)) {
            qCWarning(HealthAndArmingCheckReportLog) << "Flight mode group not set";
        }
    }
    _supported = true;

    const events::HealthAndArmingChecks::Results &results = eventHandler.healthAndArmingChecks()->results();

    QList<Check> checks;
    for (const auto& check : results.checks()) {
        QString severity = "";
        if (events::externalLogLevel(check.log_levels) <= events::Log::Error) {
            severity = "error";
        } else if (events::externalLogLevel(check.log_levels) <= events::Log::Warning) {
            severity = "warning";
        }

        QString description = QString::fromStdString(check.description);
        checks.append(Check{
            .ackKey = (static_cast<quint64>(check.id) << 32) | check.affected_health_component_index,
            .message = QString::fromStdString(check.message),
            .description = description.replace("\n", "<br/>"),
            .severity = severity,
            // Without a current mode every check is listed, since any of them might apply
            .inCurrentMode = !currentModeKnown || ((check.affected_mode_groups & (1ull << flightModeGroup)) != 0),
        });
    }
    _rebuild(checks);

    _modeGroupResults.currentModeGroup = flightModeGroup;
    _modeGroupResults.canArm = 0;
    _modeGroupResults.canRun = 0;
    for (int modeGroup = 0; modeGroup < MaxModeGroups; modeGroup++) {
        const quint64 modeGroupBit = 1ull << modeGroup;
        if (results.canArm(modeGroup)) {
            _modeGroupResults.canArm |= modeGroupBit;
        }
        if (results.canRun(modeGroup)) {
            _modeGroupResults.canRun |= modeGroupBit;
        }
    }
    qCDebug(HealthAndArmingCheckReportLog)
        << "currentModeGroup:" << flightModeGroup << "canArm:" << Qt::hex << _modeGroupResults.canArm
        << "canRun:" << _modeGroupResults.canRun << Qt::dec << "checks:" << checks.count()
        << "currentModeProblems:" << _problemsForCurrentMode->count();

    const auto& healthComponents = results.healthComponents().health_components;

    // GPS state
    const auto gpsStateIter = healthComponents.find("gps");
    if (gpsStateIter != healthComponents.end()) {
        const events::HealthAndArmingChecks::HealthComponent& gpsState = gpsStateIter->second;
        if (gpsState.health.error || gpsState.arming_check.error) {
            _gpsState = "red";
        } else if (gpsState.health.warning || gpsState.arming_check.warning) {
            _gpsState = "yellow";
        } else {
            _gpsState = "green";
        }
    }

    emit updated();
}

void HealthAndArmingCheckReport::acknowledgeAllWarnings()
{
    if (_currentModeWarningKeys.isEmpty()) {
        return;
    }

    _acknowledgedWarningKeys.unite(_currentModeWarningKeys);
    for (int i = 0; i < _problemsForCurrentMode->count(); i++) {
        auto* const problem = _problemsForCurrentMode->value<HealthAndArmingCheckProblem*>(i);
        if (problem && (problem->severity() == QStringLiteral("warning"))) {
            problem->setAcknowledged(true);
        }
    }

    emit updated();
}

void HealthAndArmingCheckReport::_rebuild(const QList<Check>& checks)
{
    // Checks are matched by key rather than text, so a warning whose values change stays acknowledged.
    // Mode-independent so acknowledgements survive a mode switch.
    QSet<quint64> reportedWarningKeys;
    for (const Check& check : checks) {
        if (check.severity == QStringLiteral("warning")) {
            reportedWarningKeys.insert(check.ackKey);
        }
    }
    _acknowledgedWarningKeys.intersect(reportedWarningKeys);

    _problemsForCurrentMode->clearAndDeleteContents();
    _currentModeWarningKeys.clear();
    for (const Check& check : checks) {
        if (!check.inCurrentMode) {
            continue;
        }

        auto* const problem = new HealthAndArmingCheckProblem(check.message, check.description, check.severity, this);
        if (check.severity == QStringLiteral("warning")) {
            _currentModeWarningKeys.insert(check.ackKey);
            problem->setAcknowledged(_acknowledgedWarningKeys.contains(check.ackKey));
        }
        _problemsForCurrentMode->append(problem);
    }
}

#ifdef QGC_UNITTEST_BUILD
void HealthAndArmingCheckReport::setProblemsForTest(const QStringList& severities)
{
    _supported = true;
    QList<Check> checks;
    for (const QString& severity : severities) {
        checks.append(Check{
            .ackKey = static_cast<quint64>(checks.count()),
            .message = QStringLiteral("Test problem"),
            .severity = severity,
            .inCurrentMode = true,
        });
    }
    _rebuild(checks);
    emit updated();
}
#endif

int HealthAndArmingCheckReport::_countProblems(const QString& severity) const
{
    int count = 0;
    for (int i = 0; i < _problemsForCurrentMode->count(); i++) {
        const auto* const problem = _problemsForCurrentMode->value<const HealthAndArmingCheckProblem*>(i);
        if (problem && (problem->severity() == severity) && !problem->acknowledged()) {
            count++;
        }
    }
    return count;
}

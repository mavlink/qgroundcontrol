#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

class EventHandler;
class QmlObjectListModel;
class HealthAndArmingCheckReportTest;

class HealthAndArmingCheckProblem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString message      READ message        CONSTANT)
    Q_PROPERTY(QString description  READ description    CONSTANT)
    Q_PROPERTY(QString severity     READ severity       CONSTANT)
    Q_PROPERTY(bool expanded        READ expanded       WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(bool acknowledged READ acknowledged NOTIFY acknowledgedChanged)

public:
    HealthAndArmingCheckProblem(const QString& message, const QString& description, const QString& severity, QObject *parent = nullptr);

    const QString& message() const { return _message; }
    const QString& description() const { return _description; }
    const QString& severity() const { return _severity; }

    bool expanded() const { return _expanded; }
    void setExpanded(bool expanded) { _expanded = expanded; emit expandedChanged(); }

    bool acknowledged() const { return _acknowledged; }

    void setAcknowledged(bool acknowledged);

signals:
    void expandedChanged();
    void acknowledgedChanged();

private:
    const QString _message;
    const QString _description;
    const QString _severity;
    bool _expanded{false};
    bool _acknowledged{false};
};

/*===========================================================================*/

class HealthAndArmingCheckReport : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(bool supported                              READ supported               NOTIFY updated)
    Q_PROPERTY(bool hasWarningsOrErrors                    READ hasWarningsOrErrors     NOTIFY updated)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY updated)
    Q_PROPERTY(int warningCount READ warningCount NOTIFY updated)
    Q_PROPERTY(QString gpsState                            READ gpsState                NOTIFY updated)
    Q_PROPERTY(QmlObjectListModel* problemsForCurrentMode  READ problemsForCurrentMode  NOTIFY updated)

public:
    explicit HealthAndArmingCheckReport(QObject *parent = nullptr);
    ~HealthAndArmingCheckReport() override;

    /// Mode group results from the latest report. The current mode group is -1 until a report arrives, and while the
    /// current mode is not in the vehicle's metadata.
    struct ModeGroupResults
    {
        int currentModeGroup = -1;
        quint64 canArm = 0;  ///< One bit per mode group: arming in that group would be accepted
        quint64 canRun = 0;  ///< One bit per mode group: switching into that group while armed would be accepted
    };

    static constexpr int MaxModeGroups = 64;

    bool supported() const { return _supported; }

    const ModeGroupResults& modeGroupResults() const { return _modeGroupResults; }

    bool hasWarningsOrErrors() const { return (errorCount() > 0) || (warningCount() > 0); }

    int errorCount() const { return _countProblems(QStringLiteral("error")); }

    /// Unacknowledged warnings only
    int warningCount() const { return _countProblems(QStringLiteral("warning")); }
    const QString& gpsState() const { return _gpsState; }
    QmlObjectListModel* problemsForCurrentMode() { return _problemsForCurrentMode; }

    void update(uint8_t compid, const EventHandler &eventHandler, int flightModeGroup);

    /// Acknowledges the warnings for the current mode so they no longer count toward the warning state.
    /// An acknowledgement is dropped once its check stops being a warning or is missing from a report.
    Q_INVOKABLE void acknowledgeAllWarnings();

#ifdef QGC_UNITTEST_BUILD
    /// Test-only: marks the report supported and replaces its problems with one per given severity
    /// ("error", "warning" or ""), so UI tests can drive health state without the events protocol.
    void setProblemsForTest(const QStringList& severities);
#endif

signals:
    void updated();

private:
    friend class HealthAndArmingCheckReportTest;

    /// A check from a full health report, decoded from libevents
    struct Check
    {
        quint64 ackKey = 0;  ///< Event id and health component index; ignores the values filled into the text
        // Default member initializers keep GCC's -Wmissing-field-initializers quiet for designated initializers
        QString message{};
        QString description{};
        QString severity{};  ///< "error", "warning" or "" (info)
        bool inCurrentMode = false;
    };

    void _rebuild(const QList<Check>& checks);
    int _countProblems(const QString& severity) const;

    bool _supported{false};
    QString _gpsState{};
    ModeGroupResults _modeGroupResults;

    QmlObjectListModel* _problemsForCurrentMode = nullptr; ///< list of HealthAndArmingCheckProblem*
    QSet<quint64> _currentModeWarningKeys;
    QSet<quint64> _acknowledgedWarningKeys;
};

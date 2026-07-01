/****************************************************************************
 * FailureInjection.h
 *
 * QML singleton backing the Failure Injection page. It serves two roles:
 *
 *  1. Static catalog: the MAVLink FAILURE_UNIT / FAILURE_TYPE enums. Enum
 *     values come from the MAVLink dialect headers, tracking common.xml;
 *     display names and per-unit instance maxima are defined in the .cc.
 *
 *  2. Session state: the activity log and the set of units injected this
 *     session, persisted here so they survive navigating away from and back
 *     to the page, which destroys/recreates the page via its SetupPage Loader.
 ****************************************************************************/
#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

class FailureInjection : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FailureInjection)
    QML_SINGLETON
    Q_PROPERTY(QVariantList units     READ units     CONSTANT)               ///< [{ name, unit, max }]
    Q_PROPERTY(QVariantList types     READ types     CONSTANT)               ///< [{ name, type }]
    Q_PROPERTY(QVariantList activity  READ activity  NOTIFY activityChanged) ///< newest first: [{ time, unitName, typeName, instance, result }]

public:
    explicit FailureInjection(QObject *parent = nullptr);

    QVariantList units(void) const { return _units; }
    QVariantList types(void) const { return _types; }
    QVariantList activity(void) const { return _activity; }

    /// Add a log row (prepended, newest first, result "pending") without tracking the unit for Reset.
    /// instanceLabel is a display descriptor for the affected instance(s), e.g. "all", "2", "1, 3, 5".
    Q_INVOKABLE void logRow(const QString &unitName, const QString &typeName, const QString &instanceLabel, const QString &time);
    /// Record one injected failure: adds a log row and remembers the unit so Reset can restore it.
    Q_INVOKABLE void logInjection(const QString &unitName, const QString &typeName, int unitEnum, const QString &instanceLabel, const QString &time);
    /// Resolve the oldest still-pending injection with a MAV_RESULT ack code; sets the row result.
    Q_INVOKABLE void resolveResult(int ackResult);
    /// Distinct FAILURE_UNIT values injected this session, so Reset restores only those.
    Q_INVOKABLE QVariantList injectedUnits(void) const;
    /// Forget the injected-units set (after a Reset reverted them); the activity log is left intact.
    Q_INVOKABLE void clearInjectedUnits(void);

signals:
    void activityChanged(void);

private:
    QVariantList _units;
    QVariantList _types;
    QVariantList _activity;
    QList<int>   _injectedUnits;
};

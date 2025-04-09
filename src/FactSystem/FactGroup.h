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
#include <QtCore/QJsonArray>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include "Fact.h"
#include "MAVLinkLib.h"

class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(FactGroupLog)

/// Used to group Facts together into an object hierarachy.
class FactGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList  factNames           READ factNames          NOTIFY factNamesChanged)
    Q_PROPERTY(QStringList  factGroupNames      READ factGroupNames     NOTIFY factGroupNamesChanged)
    Q_PROPERTY(bool         telemetryAvailable  READ telemetryAvailable NOTIFY telemetryAvailableChanged)   ///< false: No telemetry for these values has been received

public:
    explicit FactGroup(int updateRateMsecs, const QString &metaDataFile, QObject *parent = nullptr, bool ignoreCamelCase = false);
    explicit FactGroup(int updateRateMsecs, QObject *parent = nullptr, bool ignoreCamelCase = false);
    virtual ~FactGroup();

    /// @ return true: if the fact exists in the group
    Q_INVOKABLE bool factExists(const QString &name) const;

    /// @return Fact for specified name, NULL if not found
    /// Note: Requesting a fact which doesn't exists is considered an internal error and will spit out a qWarning
    Q_INVOKABLE Fact *getFact(const QString &name) const;

    /// @return FactGroup for specified name, NULL if not found
    /// Note: Requesting a fact group which doesn't exists is considered an internal error and will spit out a qWarning
    Q_INVOKABLE FactGroup *getFactGroup(const QString &name) const;

    /// Turning on live updates will allow value changes to flow through as they are received.
    Q_INVOKABLE void setLiveUpdates(bool liveUpdates);

    QStringList factNames() const { return _factNames; }
    QStringList factGroupNames() const { return _nameToFactGroupMap.keys(); }
    bool telemetryAvailable() const { return _telemetryAvailable; }
    const QMap<QString, FactGroup*> &factGroups() const { return _nameToFactGroupMap; }

    /// Allows a FactGroup to parse incoming messages and fill in values
    virtual void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) {}

signals:
    void factNamesChanged();
    void factGroupNamesChanged();
    void telemetryAvailableChanged(bool telemetryAvailable);

protected slots:
    virtual void _updateAllValues();

protected:
    void _addFact(Fact *fact, const QString &name);
    void _addFact(Fact *fact) { _addFact(fact, fact->name()); }
    void _addFactGroup(FactGroup *factGroup, const QString &name);
    void _addFactGroup(FactGroup *factGroup) { _addFactGroup(factGroup, factGroup->objectName()); }
    void _loadFromJsonArray(const QJsonArray &jsonArray);
    void _setTelemetryAvailable(bool telemetryAvailable);

    const int _updateRateMSecs = 0;   ///< Update rate for Fact::valueChanged signals, 0: immediate update

    QMap<QString, Fact*> _nameToFactMap;
    QMap<QString, FactGroup*> _nameToFactGroupMap;
    QMap<QString, FactMetaData*> _nameToFactMetaDataMap;
    QStringList _factNames;

private:
    void _setupTimer();
    static QString _camelCase(const QString &text);

    QTimer _updateTimer;
    const bool _ignoreCamelCase = false;
    bool _telemetryAvailable = false;
};

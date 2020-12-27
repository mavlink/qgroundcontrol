/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Fact.h"
#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"

#include <QStringList>
#include <QMap>
#include <QTimer>

class Vehicle;

/// Used to group Facts together into an object hierarachy.
class FactGroup : public QObject
{
    Q_OBJECT
    
public:
    FactGroup(int updateRateMsecs, const QString& metaDataFile, QObject* parent = nullptr, bool ignoreCamelCase = false);
    FactGroup(int updateRateMsecs, QObject* parent = nullptr, bool ignoreCamelCase = false);

    Q_PROPERTY(QStringList  factNames           READ factNames          NOTIFY factNamesChanged)
    Q_PROPERTY(QStringList  factGroupNames      READ factGroupNames     NOTIFY factGroupNamesChanged)
    Q_PROPERTY(bool         telemetryAvailable  READ telemetryAvailable NOTIFY telemetryAvailableChanged)   ///< false: No telemetry for these values has been received

    /// @ return true: if the fact exists in the group
    Q_INVOKABLE bool factExists(const QString& name);

    /// @return Fact for specified name, NULL if not found
    /// Note: Requesting a fact which doesn't exists is considered an internal error and will spit out a qWarning
    Q_INVOKABLE Fact* getFact(const QString& name);

    /// @return FactGroup for specified name, NULL if not found
    /// Note: Requesting a fact group which doesn't exists is considered an internal error and will spit out a qWarning
    Q_INVOKABLE FactGroup* getFactGroup(const QString& name);

    /// Turning on live updates will allow value changes to flow through as they are received.
    Q_INVOKABLE void setLiveUpdates(bool liveUpdates);

    QStringList factNames           (void) const { return _factNames; }
    QStringList factGroupNames      (void) const { return _nameToFactGroupMap.keys(); }
    bool        telemetryAvailable  (void) const { return _telemetryAvailable; }
    const QMap<QString, FactGroup*>& factGroups() const { return _nameToFactGroupMap; }

    /// Allows a FactGroup to parse incoming messages and fill in values
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message);

signals:
    void factNamesChanged           (void);
    void factGroupNamesChanged      (void);
    void telemetryAvailableChanged  (bool telemetryAvailable);

protected slots:
    virtual void _updateAllValues(void);

protected:
    void _addFact               (Fact* fact, const QString& name);
    void _addFactGroup          (FactGroup* factGroup, const QString& name);
    void _loadFromJsonArray     (const QJsonArray jsonArray);
    void _setTelemetryAvailable (bool telemetryAvailable);

    int  _updateRateMSecs;   ///< Update rate for Fact::valueChanged signals, 0: immediate update

    QMap<QString, Fact*>            _nameToFactMap;
    QMap<QString, FactGroup*>       _nameToFactGroupMap;
    QMap<QString, FactMetaData*>    _nameToFactMetaDataMap;
    QStringList                     _factNames;

private:
    void    _setupTimer (void);
    QString _camelCase  (const QString& text);

    bool    _ignoreCamelCase    = false;
    QTimer  _updateTimer;
    bool    _telemetryAvailable = false;
};

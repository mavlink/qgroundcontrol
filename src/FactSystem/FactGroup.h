/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef FactGroup_H
#define FactGroup_H

#include "Fact.h"
#include "QGCLoggingCategory.h"

#include <QStringList>
#include <QMap>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

/// Used to group Facts together into an object hierarachy.
class FactGroup : public QObject
{
    Q_OBJECT
    
public:
    FactGroup(int updateRateMsecs, const QString& metaDataFile, QObject* parent = nullptr);
    FactGroup(int updateRateMsecs, QObject* parent = nullptr);

    Q_PROPERTY(QStringList factNames        READ factNames      CONSTANT)
    Q_PROPERTY(QStringList factGroupNames   READ factGroupNames CONSTANT)

    /// @return Fact for specified name, NULL if not found
    Q_INVOKABLE Fact* getFact(const QString& name);

    /// @return FactGroup for specified name, NULL if not found
    Q_INVOKABLE FactGroup* getFactGroup(const QString& name);

    /// Turning on live updates will allow value changes to flow through as they are received.
    Q_INVOKABLE void setLiveUpdates(bool liveUpdates);

    QStringList factNames(void) const { return _factNames; }
    QStringList factGroupNames(void) const { return _nameToFactGroupMap.keys(); }

protected:
    void _addFact(Fact* fact, const QString& name);
    void _addFactGroup(FactGroup* factGroup, const QString& name);
    void _loadFromJsonArray(const QJsonArray jsonArray);

    int _updateRateMSecs;   ///< Update rate for Fact::valueChanged signals, 0: immediate update

protected slots:
    virtual void _updateAllValues(void);

private:
    void _setupTimer();
    QTimer _updateTimer;

protected:
    QMap<QString, Fact*>            _nameToFactMap;
    QMap<QString, FactGroup*>       _nameToFactGroupMap;
    QMap<QString, FactMetaData*>    _nameToFactMetaDataMap;
    QStringList                     _factNames;
};

#endif

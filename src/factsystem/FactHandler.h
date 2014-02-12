#ifndef FACTHANDLER_H
#define FACTHANDLER_H

#include "Fact.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringListIterator>
#include <QXmlStreamReader>

class UASInterface;
class FactRuleHandler;
class FactMavShim;

/// Stores the current set of facts. There are currently three types of facts:
///     Parameter data - uas parameters, prefixed with 'P.'
///     Telemetry data - telemetry values coming back from uas through mavlink, prefixed with 'T.'
///     Rules data - values calculated using Lua scripts, prefixed with 'R.'

class FactHandler : public QObject
{
    Q_OBJECT
    
signals:
    void factUpdated(int uasId, Fact::Provider_t provider, const QString& id);

public:
    FactHandler(QObject* parent = NULL);
    void setup(UASInterface* uas);
    bool containsFact(Fact::Provider_t provider, const QString& id) const;
    Fact& getFact(Fact::Provider_t provider, const QString& id);
    QStringListIterator factIdIterator(Fact::Provider_t provider);
    const QString& getFactIdByIndex(Fact::Provider_t provider, int index) const;
    QListIterator<Fact&> factIterator(Fact::Provider_t provider);
    int getFactCount(Fact::Provider_t provider) const;
    QStringList getFactIdsForGroup(Fact::Provider_t provider, const QString& group) const;
    
private slots:
    // FactMavShim signals
    void _receivedParamValue(int uasId, const QString& rawId, quint8 type, float value, quint16 index, quint16 count);
    void _receivedTelemValue(int uasId, const QString& rawId, quint8 type, QVariant value);
    void _loadMetaData(int uasId, uint8_t autopilot, uint8_t mavType);

    // FactRuleHandler signals
    void _receivedRuleData(int uasId, const QStringList& rawIds, QList<float>& values);
    
private:
    void _loadTelemetryMetaData(void);
    void _loadXmlMetaData(const QString& filename, Fact::Provider_t provider);
    void _parseField(QXmlStreamReader& xml, Fact::Provider_t provider, const QString& factId);
    void _parseValue(QXmlStreamReader& xml, Fact::Provider_t provider, const QString& factId);
    QString _parseParam(QXmlStreamReader& xml, Fact::Provider_t provider, const QString& group);
    void _finalizeFact(Fact::Provider_t provider, const QString& factId);
    void _loadParameterDefaults(const QString& filename);
    void _removeFact(Fact::Provider_t provider, const QString& factId);
    
    QString _parseMavlinkMessage(QXmlStreamReader& xml);
    void _parseMavlinkField(QXmlStreamReader& xml, const QString& group);
    void _loadMavlinkMetaData(const QString& filename);

    UASInterface* _uas;
    QMap<Fact::Provider_t, QMap<QString, Fact> >    _providerFacts;
    QMap<Fact::Provider_t, QStringList>             _providerFactIds;
};

#endif
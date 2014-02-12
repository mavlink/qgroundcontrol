#ifndef FACTMAVSHIM_H
#define FACTMAVSHIM_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>
#include "mavlink.h"

class UASInterface;
class LinkInterface;

/// This class provides a shim between MAVLinkProtocol and the Fact System. We use a shim insteand of modifying
/// MAVLinkProtocol to reduce the initial impact of the integration of the new fact system. Eventually this should
/// go away and we will modify MAVLinkProtocol directly.
class FactMavShim : public QObject
{
    Q_OBJECT
    
signals:
    void receivedParamValue(int uasId, const QString& rawId, quint8 type, float value, quint16 index, quint16 count);
    void receivedTelemValue(int uasId, const QString& rawId, quint8 type, QVariant value);
    void mavtypeKnown(int uasId, uint8_t autopilot, uint8_t mavtype);
    
public:
    FactMavShim(QObject* parent = NULL);
    void setup(UASInterface* uas);
    
private slots:
    // Signals from MAVLinkProtocol
    void _receiveMessage(LinkInterface* link, mavlink_message_t message);
    
private:
    typedef struct {
        const char* id;
        float       value;
    } IdPlusValue;
    
    void _handleParamValue(mavlink_message_t& msg);
    void _handleHeartbeat(mavlink_message_t& msg);
    void _processTelemetryData(mavlink_message_info_t* rgMessageInfo, size_t cMessageInfo, mavlink_message_t& message);
    
    UASInterface* _uas;
        
    LinkInterface       *_link;
};

#endif
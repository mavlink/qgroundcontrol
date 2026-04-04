/****************************************************************************
 *
 * USV Payload Fact Group
 * 无人船载荷数据组 - 用于管理水质监测遥测数据
 *
 * 通信诊断: 追踪来自 sysid/compid 的消息统计
 *
 ****************************************************************************/

#pragma once

#include "FactSystem/FactGroup.h"
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>

class USVPayloadFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *voltage    READ voltage    CONSTANT)
    Q_PROPERTY(Fact *absorbance READ absorbance CONSTANT)
    Q_PROPERTY(Fact *pumpX      READ pumpX      CONSTANT)
    Q_PROPERTY(Fact *pumpY      READ pumpY      CONSTANT)
    Q_PROPERTY(Fact *pumpZ      READ pumpZ      CONSTANT)
    Q_PROPERTY(Fact *pumpA      READ pumpA      CONSTANT)
    Q_PROPERTY(Fact *status     READ status     CONSTANT)
    Q_PROPERTY(Fact *linkActive   READ linkActive   CONSTANT)
    Q_PROPERTY(Fact *packetCount  READ packetCount  CONSTANT)

    // 通信诊断属性
    Q_PROPERTY(int rxMsgTotal     READ rxMsgTotal     NOTIFY diagnosticsChanged)
    Q_PROPERTY(int rxNamedValue   READ rxNamedValue   NOTIFY diagnosticsChanged)
    Q_PROPERTY(int rxHeartbeat    READ rxHeartbeat    NOTIFY diagnosticsChanged)
    Q_PROPERTY(int lastSysid      READ lastSysid      NOTIFY diagnosticsChanged)
    Q_PROPERTY(int lastCompid     READ lastCompid     NOTIFY diagnosticsChanged)
    Q_PROPERTY(double latencyMs   READ latencyMs      NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString diagSummary READ diagSummary   NOTIFY diagnosticsChanged)

public:
    explicit USVPayloadFactGroup(QObject *parent = nullptr);

    Fact *voltage()    { return &_voltageFact; }
    Fact *absorbance() { return &_absorbanceFact; }
    Fact *pumpX()      { return &_pumpXFact; }
    Fact *pumpY()      { return &_pumpYFact; }
    Fact *pumpZ()      { return &_pumpZFact; }
    Fact *pumpA()      { return &_pumpAFact; }
    Fact *status()     { return &_statusFact; }
    Fact *linkActive()   { return &_linkActiveFact; }
    Fact *packetCount()  { return &_packetCountFact; }

    // 诊断 getters
    int rxMsgTotal()   const { return _rxMsgTotal; }
    int rxNamedValue() const { return _rxNamedValue; }
    int rxHeartbeat()  const { return _rxHeartbeat; }
    int lastSysid()    const { return _lastSysid; }
    int lastCompid()   const { return _lastCompid; }
    double latencyMs() const { return _latencyMs; }
    QString diagSummary() const;

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

signals:
    void diagnosticsChanged();

private slots:
    void _telemetryTimeout();

private:
    void _handleNamedValueFloat(const mavlink_message_t &message);
    void _handleDebugVect(const mavlink_message_t &message);
    void _handleDebug(const mavlink_message_t &message);

    Fact _voltageFact;
    Fact _absorbanceFact;
    Fact _pumpXFact;
    Fact _pumpYFact;
    Fact _pumpZFact;
    Fact _pumpAFact;
    Fact _statusFact;
    Fact _linkActiveFact;
    Fact _packetCountFact;
    QTimer _timeoutTimer;
    static constexpr int _timeoutMsecs = 5000;

    // 通信诊断计数
    int _rxMsgTotal  = 0;
    int _rxNamedValue = 0;
    int _rxHeartbeat  = 0;
    int _lastSysid    = 0;
    int _lastCompid   = 0;
    double _latencyMs = 0.0;
    QElapsedTimer _latencyTimer;
};

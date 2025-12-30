/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroupListModel.h"

class EscStatusFactGroupListModel : public FactGroupListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit EscStatusFactGroupListModel(QObject* parent = nullptr);

protected:
    // Overrides from FactGroupListModel
    bool _shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const final;
    FactGroupWithId *_createFactGroupWithId(uint32_t id) final;
};

class EscStatusFactGroup : public FactGroupWithId
{
    Q_OBJECT

    Q_PROPERTY(Fact *rpm            READ rpm            CONSTANT)
    Q_PROPERTY(Fact *current        READ current        CONSTANT)
    Q_PROPERTY(Fact *voltage        READ voltage        CONSTANT)
    Q_PROPERTY(Fact *count          READ count          CONSTANT)
    Q_PROPERTY(Fact *connectionType READ connectionType CONSTANT)
    Q_PROPERTY(Fact *info           READ info           CONSTANT)
    Q_PROPERTY(Fact *failureFlags   READ failureFlags   CONSTANT)
    Q_PROPERTY(Fact *errorCount     READ errorCount     CONSTANT)
    Q_PROPERTY(Fact *temperature    READ temperature    CONSTANT)

public:
    explicit EscStatusFactGroup(uint32_t escIndex, QObject *parent = nullptr);

    Fact *rpm() { return &_rpmFact; }
    Fact *current() { return &_currentFact; }
    Fact *voltage() { return &_voltageFact; }
    Fact *count() { return &_countFact; }
    Fact *connectionType() { return &_connectionTypeFact; }
    Fact *info() { return &_infoFact; }
    Fact *failureFlags() { return &_failureFlagsFact; }
    Fact *errorCount() { return &_errorCountFact; }
    Fact *temperature() { return &_temperatureFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    void _handleEscInfo(Vehicle *vehicle, const mavlink_message_t &message);
    void _handleEscStatus(Vehicle *vehicle, const mavlink_message_t &message);

    Fact _rpmFact =             Fact(0, QStringLiteral("rpm"),              FactMetaData::valueTypeInt32);
    Fact _currentFact =         Fact(0, QStringLiteral("current"),          FactMetaData::valueTypeFloat);
    Fact _voltageFact =         Fact(0, QStringLiteral("voltage"),          FactMetaData::valueTypeFloat);
    Fact _countFact =           Fact(0, QStringLiteral("count"),            FactMetaData::valueTypeUint8);
    Fact _connectionTypeFact =  Fact(0, QStringLiteral("connectionType"),   FactMetaData::valueTypeUint8);
    Fact _infoFact =            Fact(0, QStringLiteral("info"),             FactMetaData::valueTypeUint8);
    Fact _failureFlagsFact =    Fact(0, QStringLiteral("failureFlags"),     FactMetaData::valueTypeUint16);
    Fact _errorCountFact =      Fact(0, QStringLiteral("errorCount"),       FactMetaData::valueTypeUint32);
    Fact _temperatureFact =     Fact(0, QStringLiteral("temperature"),      FactMetaData::valueTypeFloat);
};

/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"

class QmlObjectListModel;

Q_DECLARE_LOGGING_CATEGORY(MAVLinkMessageLog)

class QGCMAVLinkMessage : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(quint32              id              READ id             CONSTANT)
    Q_PROPERTY(quint32              sysId           READ sysId          CONSTANT)
    Q_PROPERTY(quint32              compId          READ compId         CONSTANT)
    Q_PROPERTY(QString              name            READ name           CONSTANT)
    Q_PROPERTY(qreal                actualRateHz    READ actualRateHz   NOTIFY actualRateHzChanged)
    Q_PROPERTY(int32_t              targetRateHz    READ targetRateHz   NOTIFY targetRateHzChanged)
    Q_PROPERTY(quint64              count           READ count          NOTIFY countChanged)
    Q_PROPERTY(QmlObjectListModel   *fields         READ fields         CONSTANT)
    Q_PROPERTY(bool                 fieldSelected   READ fieldSelected  NOTIFY fieldSelectedChanged)
    Q_PROPERTY(bool                 selected        READ selected       NOTIFY selectedChanged)

public:
    explicit QGCMAVLinkMessage(const mavlink_message_t &message, QObject *parent = nullptr);
    ~QGCMAVLinkMessage();

    quint32 id() const { return _message.msgid;  }
    quint8 sysId() const { return _message.sysid; }
    quint8 compId() const { return _message.compid; }
    QString name() const { return _name;  }
    qreal actualRateHz() const { return _actualRateHz; }
    int32_t targetRateHz() const { return _targetRateHz; }
    quint64 count() const { return _count; }
    quint64 lastCount() const { return _lastCount; }
    QmlObjectListModel *fields() const { return _fields; }
    bool fieldSelected() const { return _fieldSelected; }
    bool selected() const { return _selected; }

    void updateFieldSelection();
    void update(const mavlink_message_t &message);
    void updateFreq();
    void setSelected(bool sel);
    void setTargetRateHz(int32_t rate);

signals:
    void countChanged();
    void actualRateHzChanged();
    void targetRateHzChanged();
    void fieldSelectedChanged();
    void selectedChanged();

private:
    void _updateFields();

    mavlink_message_t _message{};
    QmlObjectListModel *_fields = nullptr;
    QString _name;
    qreal _actualRateHz = 0.0;
    int32_t _targetRateHz = 0;
    uint64_t _count = 1;
    uint64_t _lastCount = 0;
    bool _fieldSelected = false;
    bool _selected = false;
};

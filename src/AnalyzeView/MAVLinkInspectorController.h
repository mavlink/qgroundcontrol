/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkProtocol.h"
#include "Vehicle.h"

#include <QObject>
#include <QString>
#include <QDebug>
#include <QAbstractListModel>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkInspectorLog)

//-----------------------------------------------------------------------------
class QGCMAVLinkMessageField : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString      name        READ name       CONSTANT)
    Q_PROPERTY(QString      type        READ type       CONSTANT)
    Q_PROPERTY(QString      value       READ value      NOTIFY valueChanged)

public:
    QGCMAVLinkMessageField(QObject* parent, QString name, QString type);

    QString     name            () { return _name;  }
    QString     type            () { return _type;  }
    QString     value           () { return _value; }

    void        updateValue     (QString newValue) { _value = newValue; emit valueChanged(); }

signals:
    void        valueChanged    ();

private:
    QString     _type;
    QString     _name;
    QString     _value;
};

//-----------------------------------------------------------------------------
class QGCMAVLinkMessage : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint32              id          READ id         CONSTANT)
    Q_PROPERTY(QString              name        READ name       CONSTANT)
    Q_PROPERTY(quint64              messageHz   READ messageHz  NOTIFY messageChanged)
    Q_PROPERTY(quint64              count       READ count      NOTIFY messageChanged)
    Q_PROPERTY(QmlObjectListModel*  fields      READ fields     CONSTANT)

public:
    QGCMAVLinkMessage(QObject* parent, mavlink_message_t* message);

    quint32             id          () { return _message.msgid; }
    QString             name        () { return _name;  }
    quint64             messageHz   () { return _messageHz; }
    quint64             count       () { return _count; }
    QmlObjectListModel* fields      () { return &_fields; }

    void                update      (mavlink_message_t* message);

signals:
    void messageChanged             ();

private:
    QmlObjectListModel  _fields;
    QString             _name;
    uint64_t            _messageHz  = 0;
    uint64_t            _count      = 0;
    mavlink_message_t   _message;   //-- List of QGCMAVLinkMessageField
};

//-----------------------------------------------------------------------------
class QGCMAVLinkVehicle : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint8               id          READ id         CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  messages    READ messages   NOTIFY messagesChanged)

public:
    QGCMAVLinkVehicle(QObject* parent, quint8 id);

    quint8              id          () { return _id; }
    QmlObjectListModel* messages    () { return &_messages; }

    QGCMAVLinkMessage*  findMessage (uint32_t id);
    void                append      (QGCMAVLinkMessage* message);

signals:
    void messagesChanged            ();

private:
    quint8              _id;
    QmlObjectListModel  _messages;  //-- List of QGCMAVLinkMessage
};

//-----------------------------------------------------------------------------
class MAVLinkInspectorController : public QObject
{
    Q_OBJECT
public:
    MAVLinkInspectorController();
    ~MAVLinkInspectorController();

    Q_PROPERTY(QStringList          vehicleNames    READ vehicleNames   NOTIFY vehiclesChanged)
    Q_PROPERTY(QmlObjectListModel*  vehicles        READ vehicles       NOTIFY vehiclesChanged)

    QmlObjectListModel*  vehicles       () { return &_vehicles; }
    QStringList          vehicleNames   () { return _vehicleNames; }

signals:
    void vehiclesChanged                ();

private slots:
    void _receiveMessage                (LinkInterface* link, mavlink_message_t message);
    void _vehicleAdded                  (Vehicle* vehicle);
    void _vehicleRemoved                (Vehicle* vehicle);

private:
    void _reset                         ();

    QGCMAVLinkVehicle*  _findVehicle    (uint8_t id);

private:
    int         _selectedSystemID       = 0;                    ///< Currently selected system
    int         _selectedComponentID    = 0;                    ///< Currently selected component

    QStringList         _vehicleNames;
    QmlObjectListModel  _vehicles;  //-- List of QGCMAVLinkVehicle
};

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
    Q_PROPERTY(quint32              id          READ id         NOTIFY indexChanged)
    Q_PROPERTY(quint32              cid         READ cid        NOTIFY indexChanged)
    Q_PROPERTY(QString              name        READ name       NOTIFY indexChanged)
    Q_PROPERTY(qreal                messageHz   READ messageHz  NOTIFY freqChanged)
    Q_PROPERTY(quint64              count       READ count      NOTIFY messageChanged)
    Q_PROPERTY(QmlObjectListModel*  fields      READ fields     NOTIFY indexChanged)

public:
    QGCMAVLinkMessage(QObject* parent, mavlink_message_t* message);

    quint32             id          () { return _message.msgid;  }
    quint8              cid         () { return _message.compid; }
    QString             name        () { return _name;  }
    qreal               messageHz   () { return _messageHz; }
    quint64             count       () { return _count; }
    quint64             lastCount   () { return _lastCount; }
    QmlObjectListModel* fields      () { return &_fields; }

    void                update      (mavlink_message_t* message);
    void                updateFreq  ();

signals:
    void messageChanged             ();
    void freqChanged                ();
    void indexChanged               ();

private:
    QmlObjectListModel  _fields;
    QString             _name;
    qreal               _messageHz  = 0.0;
    uint64_t            _count      = 0;
    uint64_t            _lastCount  = 0;
    mavlink_message_t   _message;   //-- List of QGCMAVLinkMessageField
};

//-----------------------------------------------------------------------------
class QGCMAVLinkVehicle : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint8               id              READ id             CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  messages        READ messages       NOTIFY messagesChanged)
    Q_PROPERTY(QList<int>           compIDs         READ compIDs        NOTIFY compIDsChanged)
    Q_PROPERTY(QStringList          compIDsStr      READ compIDsStr     NOTIFY compIDsChanged)

public:
    QGCMAVLinkVehicle(QObject* parent, quint8 id);

    quint8              id              () { return _id; }
    QmlObjectListModel* messages        () { return &_messages; }
    QList<int>          compIDs         () { return _compIDs; }
    QStringList         compIDsStr      () { return _compIDsStr; }

    QGCMAVLinkMessage*  findMessage     (uint32_t id, uint8_t cid);
    void                append          (QGCMAVLinkMessage* message);

signals:
    void messagesChanged                ();
    void compIDsChanged                 ();

private:
    void _checkCompID                   (QGCMAVLinkMessage *message);

private:
    quint8              _id;
    QList<int>          _compIDs;
    QStringList         _compIDsStr;
    QmlObjectListModel  _messages;      //-- List of QGCMAVLinkMessage
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
    Q_PROPERTY(QGCMAVLinkVehicle*   activeVehicle   READ activeVehicle  NOTIFY activeVehiclesChanged)

    QmlObjectListModel*  vehicles       () { return &_vehicles;     }
    QGCMAVLinkVehicle*   activeVehicle  () { return _activeVehicle; }
    QStringList          vehicleNames   () { return _vehicleNames;  }

signals:
    void vehiclesChanged                ();
    void activeVehiclesChanged          ();

private slots:
    void _receiveMessage                (LinkInterface* link, mavlink_message_t message);
    void _vehicleAdded                  (Vehicle* vehicle);
    void _vehicleRemoved                (Vehicle* vehicle);
    void _setActiveVehicle              (Vehicle* vehicle);
    void _refreshFrequency              ();

private:
    void _reset                         ();

    QGCMAVLinkVehicle*  _findVehicle    (uint8_t id);

private:
    int         _selectedSystemID       = 0;                    ///< Currently selected system
    int         _selectedComponentID    = 0;                    ///< Currently selected component

    QGCMAVLinkVehicle*  _activeVehicle = nullptr;
    QTimer              _updateTimer;
    QStringList         _vehicleNames;
    QmlObjectListModel  _vehicles;  //-- List of QGCMAVLinkVehicle
};

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
#include <QVariantList>
#include <QtCharts/QAbstractSeries>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkInspectorLog)

QT_CHARTS_USE_NAMESPACE

class QGCMAVLinkMessage;
class MAVLinkInspectorController;

//-----------------------------------------------------------------------------
class QGCMAVLinkMessageField : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString      name        READ name       CONSTANT)
    Q_PROPERTY(QString      label       READ label      CONSTANT)
    Q_PROPERTY(QString      type        READ type       CONSTANT)
    Q_PROPERTY(QString      value       READ value      NOTIFY valueChanged)
    Q_PROPERTY(qreal        rangeMin    READ rangeMin   NOTIFY rangeMinChanged)
    Q_PROPERTY(qreal        rangeMax    READ rangeMax   NOTIFY rangeMaxChanged)
    Q_PROPERTY(bool         selectable  READ selectable NOTIFY selectableChanged)
    Q_PROPERTY(bool         selected    READ selected   WRITE setSelected   NOTIFY selectedChanged)

public:
    QGCMAVLinkMessageField(QGCMAVLinkMessage* parent, QString name, QString type);

    QString     name            () { return _name;  }
    QString     label           ();
    QString     type            () { return _type;  }
    QString     value           () { return _value; }
    qreal       rangeMin        () { return _rangeMin; }
    qreal       rangeMax        () { return _rangeMax; }
    QList<QPointF> series       () { return _series; }
    bool        selectable      () { return _selectable; }
    bool        selected        () { return _selected; }

    void        setSelectable   (bool sel);
    void        setSelected     (bool sel);
    void        updateValue     (QString newValue, qreal v);

signals:
    void        rangeMinChanged     ();
    void        rangeMaxChanged     ();
    void        seriesChanged       ();
    void        selectableChanged   ();
    void        selectedChanged     ();
    void        valueChanged        ();

private:
    void        _updateSeries       ();

private:
    QString     _type;
    QString     _name;
    QString     _value;
    QGCMAVLinkMessage* _msg = nullptr;
    bool        _selectable = true;
    bool        _selected   = false;
    int         _dataIndex  = 0;
    qreal       _rangeMin   = 0;
    qreal       _rangeMax   = 0;
    QVector<qreal>      _values;
    QVector<quint64>    _times;
    QList<QPointF>      _series;
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
    Q_PROPERTY(bool                 selected    READ selected   NOTIFY selectedChanged)

public:
    QGCMAVLinkMessage(MAVLinkInspectorController* parent, mavlink_message_t* message);

    quint32             id          () { return _message.msgid;  }
    quint8              cid         () { return _message.compid; }
    QString             name        () { return _name;  }
    qreal               messageHz   () { return _messageHz; }
    quint64             count       () { return _count; }
    quint64             lastCount   () { return _lastCount; }
    QmlObjectListModel* fields      () { return &_fields; }
    bool                selected    () { return _selected; }

    MAVLinkInspectorController* msgCtl () { return _msgCtl; }

    void                select      ();
    void                update      (mavlink_message_t* message);
    void                updateFreq  ();

signals:
    void messageChanged             ();
    void freqChanged                ();
    void indexChanged               ();
    void selectedChanged            ();

private:
    QmlObjectListModel  _fields;
    QString             _name;
    qreal               _messageHz  = 0.0;
    uint64_t            _count      = 0;
    uint64_t            _lastCount  = 0;
    mavlink_message_t   _message;   //-- List of QGCMAVLinkMessageField
    MAVLinkInspectorController* _msgCtl = nullptr;
    bool                _selected   = false;
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

    Q_PROPERTY(QStringList          vehicleNames    READ vehicleNames       NOTIFY vehiclesChanged)
    Q_PROPERTY(QmlObjectListModel*  vehicles        READ vehicles           NOTIFY vehiclesChanged)
    Q_PROPERTY(QGCMAVLinkVehicle*   activeVehicle   READ activeVehicle      NOTIFY activeVehiclesChanged)
    Q_PROPERTY(int                  chartFieldCount READ chartFieldCount    NOTIFY chartFieldCountChanged)
    Q_PROPERTY(QVariantList         chartFields     READ chartFields        NOTIFY chartFieldCountChanged)
    Q_PROPERTY(QDateTime            rangeXMin       READ rangeXMin          NOTIFY rangeMinXChanged)
    Q_PROPERTY(QDateTime            rangeXMax       READ rangeXMax          NOTIFY rangeMaxXChanged)

    Q_PROPERTY(QStringList          timeScales      READ timeScales         CONSTANT)
    Q_PROPERTY(quint32              timeScale       READ timeScale          WRITE  setTimeScale  NOTIFY timeScaleChanged)

    Q_INVOKABLE void        updateSeries    (int index, QAbstractSeries *series);

    QmlObjectListModel*     vehicles        () { return &_vehicles;     }
    QGCMAVLinkVehicle*      activeVehicle   () { return _activeVehicle; }
    QStringList             vehicleNames    () { return _vehicleNames;  }
    quint32                 timeScale       () { return _timeScale;     }
    QStringList             timeScales      () { return _timeScales;    }
    QVariantList            chartFields     () { return _chartFields;   }
    QDateTime               rangeXMin       () { return _rangeXMin;     }
    QDateTime               rangeXMax       () { return _rangeXMax;     }

    void                    setTimeScale    (quint32 t);
    int                     chartFieldCount () { return _chartFields.count(); }
    void                    addChartField   (QGCMAVLinkMessageField* field);
    void                    delChartField   (QGCMAVLinkMessageField* field);
    void                    updateXRange    ();

signals:
    void vehiclesChanged                ();
    void activeVehiclesChanged          ();
    void chartFieldCountChanged         ();
    void timeScaleChanged               ();
    void rangeMinXChanged               ();
    void rangeMaxXChanged               ();

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
    int                 _selectedSystemID       = 0;                    ///< Currently selected system
    int                 _selectedComponentID    = 0;                    ///< Currently selected component
    QStringList         _timeScales;
    quint32             _timeScale              = 0;                    ///< 5 Seconds
    QDateTime           _rangeXMin;
    QDateTime           _rangeXMax;
    QGCMAVLinkVehicle*  _activeVehicle          = nullptr;
    QTimer              _updateTimer;
    QStringList         _vehicleNames;
    QmlObjectListModel  _vehicles;                                      ///< List of QGCMAVLinkVehicle
    QVariantList        _chartFields;
};

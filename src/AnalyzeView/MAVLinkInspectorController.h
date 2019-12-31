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
    Q_PROPERTY(QString          name        READ name       CONSTANT)
    Q_PROPERTY(QString          label       READ label      CONSTANT)
    Q_PROPERTY(QString          type        READ type       CONSTANT)
    Q_PROPERTY(QString          value       READ value      NOTIFY valueChanged)
    Q_PROPERTY(bool             selectable  READ selectable NOTIFY selectableChanged)
    Q_PROPERTY(bool             left        READ left       NOTIFY seriesChanged)
    Q_PROPERTY(QAbstractSeries* series      READ series     NOTIFY seriesChanged)

public:
    QGCMAVLinkMessageField(QGCMAVLinkMessage* parent, QString name, QString type);

    QString         name            () { return _name;  }
    QString         label           ();
    QString         type            () { return _type;  }
    QString         value           () { return _value; }
    bool            selectable      () { return _selectable; }
    bool            selected        () { return _pSeries != nullptr; }
    bool            left            () { return _left; }
    QAbstractSeries*series          () { return _pSeries; }
    QList<QPointF>* values          () { return &_values;}
    qreal           rangeMin        () { return _rangeMin; }
    qreal           rangeMax        () { return _rangeMax; }

    void            setSelectable   (bool sel);
    void            updateValue     (QString newValue, qreal v);

    void            addSeries       (QAbstractSeries* series, bool left);
    void            delSeries       ();
    void            updateSeries    ();

signals:
    void            seriesChanged       ();
    void            selectableChanged   ();
    void            valueChanged        ();

private:
    QString     _type;
    QString     _name;
    QString     _value;
    bool        _selectable = true;
    bool        _left       = false;
    int         _dataIndex  = 0;
    qreal       _rangeMin   = 0;
    qreal       _rangeMax   = 0;

    QAbstractSeries*    _pSeries = nullptr;
    QGCMAVLinkMessage*  _msg     = nullptr;
    QList<QPointF>      _values;
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

    Q_PROPERTY(QStringList          vehicleNames            READ vehicleNames           NOTIFY vehiclesChanged)
    Q_PROPERTY(QmlObjectListModel*  vehicles                READ vehicles               NOTIFY vehiclesChanged)
    Q_PROPERTY(QGCMAVLinkVehicle*   activeVehicle           READ activeVehicle          NOTIFY activeVehiclesChanged)
    Q_PROPERTY(QVariantList         rightChartFields        READ rightChartFields       NOTIFY rightChartFieldsChanged)
    Q_PROPERTY(QVariantList         leftChartFields         READ leftChartFields        NOTIFY leftChartFieldsChanged)
    Q_PROPERTY(int                  seriesCount             READ seriesCount            NOTIFY seriesCountChanged)
    Q_PROPERTY(QDateTime            rangeXMin               READ rangeXMin              NOTIFY rangeMinXChanged)
    Q_PROPERTY(QDateTime            rangeXMax               READ rangeXMax              NOTIFY rangeMaxXChanged)
    Q_PROPERTY(qreal                rightRangeMin           READ rightRangeMin          NOTIFY rightRangeMinChanged)
    Q_PROPERTY(qreal                rightRangeMax           READ rightRangeMax          NOTIFY rightRangeMaxChanged)
    Q_PROPERTY(qreal                leftRangeMin            READ leftRangeMin           NOTIFY leftRangeMinChanged)
    Q_PROPERTY(qreal                leftRangeMax            READ leftRangeMax           NOTIFY leftRangeMaxChanged)
    Q_PROPERTY(QStringList          timeScales              READ timeScales             NOTIFY timeScalesChanged)
    Q_PROPERTY(QStringList          rangeList               READ rangeList              NOTIFY rangeListChanged)

    Q_PROPERTY(quint32              leftRangeIdx            READ leftRangeIdx           WRITE setLeftRangeIdx   NOTIFY leftRangeChanged)
    Q_PROPERTY(quint32              rightRangeIdx           READ rightRangeIdx          WRITE setRightRangeIdx  NOTIFY rightRangeChanged)
    Q_PROPERTY(quint32              timeScale               READ timeScale              WRITE setTimeScale      NOTIFY timeScaleChanged)

    Q_INVOKABLE void        addSeries           (QGCMAVLinkMessageField* field, QAbstractSeries* series, bool left);
    Q_INVOKABLE void        delSeries           (QGCMAVLinkMessageField* field);

    QmlObjectListModel*     vehicles            () { return &_vehicles;     }
    QGCMAVLinkVehicle*      activeVehicle       () { return _activeVehicle; }
    QStringList             vehicleNames        () { return _vehicleNames;  }
    quint32                 timeScale           () { return _timeScale;     }
    QStringList             timeScales          ();
    int                     seriesCount         () { return _rightChartFields.count() + _leftChartFields.count(); }
    QStringList             rangeList           ();
    QVariantList            rightChartFields    () { return _rightChartFields; }
    QVariantList            leftChartFields     () { return _leftChartFields;  }
    QDateTime               rangeXMin           () { return _rangeXMin; }
    QDateTime               rangeXMax           () { return _rangeXMax; }

    qreal                   rightRangeMin       () { return _rightRangeMin;   }
    qreal                   rightRangeMax       () { return _rightRangeMax;   }
    quint32                 rightRangeIdx       () { return _rightRangeIndex; }

    qreal                   leftRangeMin        () { return _leftRangeMin;   }
    qreal                   leftRangeMax        () { return _leftRangeMax;   }
    quint32                 leftRangeIdx        () { return _leftRangeIndex; }

    void                    setTimeScale        (quint32 t);
    void                    setRightRangeIdx    (quint32 r);
    void                    setLeftRangeIdx     (quint32 r);
    void                    addChartField       (QGCMAVLinkMessageField* field, bool left);
    void                    delChartField       (QGCMAVLinkMessageField* field, bool left);
    void                    updateXRange        ();
    void                    updateYRange        (bool left);

signals:
    void vehiclesChanged                ();
    void activeVehiclesChanged          ();
    void rightChartFieldsChanged        ();
    void leftChartFieldsChanged         ();
    void timeScaleChanged               ();
    void rangeMinXChanged               ();
    void rangeMaxXChanged               ();
    void timeScalesChanged              ();
    void rangeListChanged               ();
    void rightRangeMinChanged           ();
    void rightRangeMaxChanged           ();
    void rightRangeChanged              ();
    void leftRangeMinChanged            ();
    void leftRangeMaxChanged            ();
    void leftRangeChanged               ();
    void seriesCountChanged             ();

private slots:
    void _receiveMessage                (LinkInterface* link, mavlink_message_t message);
    void _vehicleAdded                  (Vehicle* vehicle);
    void _vehicleRemoved                (Vehicle* vehicle);
    void _setActiveVehicle              (Vehicle* vehicle);
    void _refreshFrequency              ();
    void _refreshSeries                 ();

private:
    QGCMAVLinkVehicle*  _findVehicle    (uint8_t id);

private:

    class TimeScale_st : public QObject {
    public:
        TimeScale_st(QObject* parent, const QString& l, uint32_t t);
        QString     label;
        uint32_t    timeScale;
    };

    class Range_st : public QObject {
    public:
        Range_st(QObject* parent, const QString& l, qreal r);
        QString     label;
        qreal       range;
    };

    int                 _selectedSystemID       = 0;                    ///< Currently selected system
    int                 _selectedComponentID    = 0;                    ///< Currently selected component
    QStringList         _timeScales;
    QStringList         _rangeList;
    quint32             _timeScale              = 0;                    ///< 5 Seconds
    QDateTime           _rangeXMin;
    QDateTime           _rangeXMax;
    qreal               _leftRangeMin           = 0;
    qreal               _leftRangeMax           = 1;
    quint32             _leftRangeIndex         = 0;                    ///> Auto Range
    qreal               _rightRangeMin          = 0;
    qreal               _rightRangeMax          = 1;
    quint32             _rightRangeIndex        = 0;                    ///> Auto Range
    QGCMAVLinkVehicle*  _activeVehicle          = nullptr;
    QTimer              _updateFrequencyTimer;
    QTimer              _updateSeriesTimer;
    QStringList         _vehicleNames;
    QmlObjectListModel  _vehicles;                                      ///< List of QGCMAVLinkVehicle
    QVariantList        _rightChartFields;
    QVariantList        _leftChartFields;
    QList<TimeScale_st*>_timeScaleSt;
    QList<Range_st*>    _rangeSt;
};

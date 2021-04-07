/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief MAVLink message inspector and charting controller
/// @author Gus Grubba <gus@auterion.com>

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
class QGCMAVLinkSystem;
class MAVLinkChartController;
class MAVLinkInspectorController;

//-----------------------------------------------------------------------------
/// MAVLink message field
class QGCMAVLinkMessageField : public QObject {
    Q_OBJECT
public:
    Q_PROPERTY(QString          name        READ name       CONSTANT)
    Q_PROPERTY(QString          label       READ label      CONSTANT)
    Q_PROPERTY(QString          type        READ type       CONSTANT)
    Q_PROPERTY(QString          value       READ value      NOTIFY valueChanged)
    Q_PROPERTY(bool             selectable  READ selectable NOTIFY selectableChanged)
    Q_PROPERTY(int              chartIndex  READ chartIndex CONSTANT)
    Q_PROPERTY(QAbstractSeries* series      READ series     NOTIFY seriesChanged)

    QGCMAVLinkMessageField(QGCMAVLinkMessage* parent, QString name, QString type);

    QString         name            () { return _name;  }
    QString         label           ();
    QString         type            () { return _type;  }
    QString         value           () { return _value; }
    bool            selectable      () const{ return _selectable; }
    bool            selected        () { return _pSeries != nullptr; }
    QAbstractSeries*series          () { return _pSeries; }
    QList<QPointF>* values          () { return &_values;}
    qreal           rangeMin        () const{ return _rangeMin; }
    qreal           rangeMax        () const{ return _rangeMax; }
    int             chartIndex      ();

    void            setSelectable   (bool sel);
    void            updateValue     (QString newValue, qreal v);

    void            addSeries       (MAVLinkChartController* chart, QAbstractSeries* series);
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
    int         _dataIndex  = 0;
    qreal       _rangeMin   = 0;
    qreal       _rangeMax   = 0;

    QAbstractSeries*    _pSeries = nullptr;
    QGCMAVLinkMessage*  _msg     = nullptr;
    MAVLinkChartController*      _chart   = nullptr;
    QList<QPointF>      _values;
};

//-----------------------------------------------------------------------------
/// MAVLink message
class QGCMAVLinkMessage : public QObject {
    Q_OBJECT
public:
    Q_PROPERTY(quint32              id              READ id             CONSTANT)
    Q_PROPERTY(quint32              cid             READ cid            CONSTANT)
    Q_PROPERTY(QString              name            READ name           CONSTANT)
    Q_PROPERTY(qreal                messageHz       READ messageHz      NOTIFY freqChanged)
    Q_PROPERTY(quint64              count           READ count          NOTIFY countChanged)
    Q_PROPERTY(QmlObjectListModel*  fields          READ fields         CONSTANT)
    Q_PROPERTY(bool                 fieldSelected   READ fieldSelected  NOTIFY fieldSelectedChanged)
    Q_PROPERTY(bool                 selected        READ selected       NOTIFY selectedChanged)

    QGCMAVLinkMessage   (QObject* parent, mavlink_message_t* message);
    ~QGCMAVLinkMessage  ();

    quint32             id              () const{ return _message.msgid;  }
    quint8              cid             () const{ return _message.compid; }
    QString             name            () { return _name;  }
    qreal               messageHz       () const{ return _messageHz; }
    quint64             count           () const{ return _count; }
    quint64             lastCount       () const{ return _lastCount; }
    QmlObjectListModel* fields          () { return &_fields; }
    bool                fieldSelected   () const{ return _fieldSelected; }
    bool                selected        () const{ return _selected; }

    void                updateFieldSelection();
    void                update          (mavlink_message_t* message);
    void                updateFreq      ();
    void                setSelected     (bool sel);

signals:
    void countChanged                   ();
    void freqChanged                    ();
    void fieldSelectedChanged           ();
    void selectedChanged                ();

private:
    void _updateFields(void);

    QmlObjectListModel  _fields;
    QString             _name;
    qreal               _messageHz      = 0.0;
    uint64_t            _count          = 1;
    uint64_t            _lastCount      = 0;
    mavlink_message_t   _message;
    bool                _fieldSelected  = false;
    bool                _selected       = false;
};

//-----------------------------------------------------------------------------
/// Vehicle MAVLink message belongs to
class QGCMAVLinkSystem : public QObject {
    Q_OBJECT
public:
    Q_PROPERTY(quint8               id              READ id                                 CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  messages        READ messages                           CONSTANT)
    Q_PROPERTY(QList<int>           compIDs         READ compIDs                            NOTIFY compIDsChanged)
    Q_PROPERTY(QStringList          compIDsStr      READ compIDsStr                         NOTIFY compIDsChanged)
    Q_PROPERTY(int                  selected        READ selected       WRITE setSelected   NOTIFY selectedChanged)

    QGCMAVLinkSystem   (QObject* parent, quint8 id);
    ~QGCMAVLinkSystem  ();

    quint8              id              () const{ return _id; }
    QmlObjectListModel* messages        () { return &_messages; }
    QList<int>          compIDs         () { return _compIDs; }
    QStringList         compIDsStr      () { return _compIDsStr; }
    int                 selected        () const{ return _selected; }

    void                setSelected     (int sel);
    QGCMAVLinkMessage*  findMessage     (uint32_t id, uint8_t cid);
    int                 findMessage     (QGCMAVLinkMessage* message);
    void                append          (QGCMAVLinkMessage* message);

signals:
    void compIDsChanged                 ();
    void selectedChanged                ();

private:
    void _checkCompID                   (QGCMAVLinkMessage *message);
    void _resetSelection                ();

private:
    quint8              _id;
    QList<int>          _compIDs;
    QStringList         _compIDsStr;
    QmlObjectListModel  _messages;      //-- List of QGCMAVLinkMessage
    int                 _selected = 0;
};

//-----------------------------------------------------------------------------
/// MAVLink message charting controller
class MAVLinkChartController : public QObject {
    Q_OBJECT
public:
    MAVLinkChartController(MAVLinkInspectorController* parent, int index);

    Q_PROPERTY(QVariantList chartFields         READ chartFields            NOTIFY chartFieldsChanged)
    Q_PROPERTY(QDateTime    rangeXMin           READ rangeXMin              NOTIFY rangeXMinChanged)
    Q_PROPERTY(QDateTime    rangeXMax           READ rangeXMax              NOTIFY rangeXMaxChanged)
    Q_PROPERTY(qreal        rangeYMin           READ rangeYMin              NOTIFY rangeYMinChanged)
    Q_PROPERTY(qreal        rangeYMax           READ rangeYMax              NOTIFY rangeYMaxChanged)
    Q_PROPERTY(int          chartIndex          READ chartIndex             CONSTANT)

    Q_PROPERTY(quint32      rangeYIndex         READ rangeYIndex            WRITE setRangeYIndex    NOTIFY rangeYIndexChanged)
    Q_PROPERTY(quint32      rangeXIndex         READ rangeXIndex            WRITE setRangeXIndex    NOTIFY rangeXIndexChanged)

    Q_INVOKABLE void        addSeries           (QGCMAVLinkMessageField* field, QAbstractSeries* series);
    Q_INVOKABLE void        delSeries           (QGCMAVLinkMessageField* field);

    Q_INVOKABLE MAVLinkInspectorController* controller() { return _controller; }

    QVariantList            chartFields         () { return _chartFields; }
    QDateTime               rangeXMin           () { return _rangeXMin;   }
    QDateTime               rangeXMax           () { return _rangeXMax;   }
    qreal                   rangeYMin           () const{ return _rangeYMin;   }
    qreal                   rangeYMax           () const{ return _rangeYMax;   }
    quint32                 rangeXIndex         () const{ return _rangeXIndex; }
    quint32                 rangeYIndex         () const{ return _rangeYIndex; }
    int                     chartIndex          () const{ return _index; }

    void                    setRangeXIndex      (quint32 t);
    void                    setRangeYIndex      (quint32 r);
    void                    updateXRange        ();
    void                    updateYRange        ();

signals:
    void chartFieldsChanged ();
    void rangeXMinChanged   ();
    void rangeXMaxChanged   ();
    void rangeYMinChanged   ();
    void rangeYMaxChanged   ();
    void rangeYIndexChanged ();
    void rangeXIndexChanged ();

private slots:
    void _refreshSeries     ();

private:
    QTimer              _updateSeriesTimer;
    QDateTime           _rangeXMin;
    QDateTime           _rangeXMax;
    int                 _index               = 0;
    qreal               _rangeYMin           = 0;
    qreal               _rangeYMax           = 1;
    quint32             _rangeXIndex         = 0;                    ///< 5 Seconds
    quint32             _rangeYIndex         = 0;                    ///< Auto Range
    QVariantList        _chartFields;
    MAVLinkInspectorController* _controller  = nullptr;
};

//-----------------------------------------------------------------------------
/// MAVLink message inspector controller (provides the logic for UI display)
class MAVLinkInspectorController : public QObject
{
    Q_OBJECT
public:
    MAVLinkInspectorController();
    ~MAVLinkInspectorController();

    Q_PROPERTY(QStringList          systemNames     READ systemNames    NOTIFY systemsChanged)
    Q_PROPERTY(QmlObjectListModel*  systems         READ systems       NOTIFY systemsChanged)
    Q_PROPERTY(QmlObjectListModel*  charts          READ charts         NOTIFY chartsChanged)
    Q_PROPERTY(QGCMAVLinkSystem*    activeSystem    READ activeSystem   NOTIFY activeSystemChanged)
    Q_PROPERTY(QStringList          timeScales      READ timeScales     NOTIFY timeScalesChanged)
    Q_PROPERTY(QStringList          rangeList       READ rangeList      NOTIFY rangeListChanged)

    Q_INVOKABLE MAVLinkChartController* createChart     ();
    Q_INVOKABLE void                    deleteChart     (MAVLinkChartController* chart);
    Q_INVOKABLE void                    setActiveSystem (int systemId);

    QmlObjectListModel* systems     () { return &_systems;     }
    QmlObjectListModel* charts      () { return &_charts;       }
    QGCMAVLinkSystem*   activeSystem() { return _activeSystem; }
    QStringList         systemNames () { return _systemNames;  }
    QStringList         timeScales  ();
    QStringList         rangeList   ();

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

    const QList<TimeScale_st*>&     timeScaleSt         () { return _timeScaleSt; }
    const QList<Range_st*>&         rangeSt             () { return _rangeSt; }

signals:
    void systemsChanged     ();
    void chartsChanged      ();
    void activeSystemChanged();
    void timeScalesChanged  ();
    void rangeListChanged   ();

private slots:
    void _receiveMessage    (LinkInterface* link, mavlink_message_t message);
    void _vehicleAdded      (Vehicle* vehicle);
    void _vehicleRemoved    (Vehicle* vehicle);
    void _setActiveVehicle  (Vehicle* vehicle);
    void _refreshFrequency  ();

private:
    QGCMAVLinkSystem* _findVehicle (uint8_t id);

private:

    int                 _selectedSystemID       = 0;        ///< Currently selected system
    int                 _selectedComponentID    = 0;        ///< Currently selected component
    QStringList         _timeScales;
    QStringList         _rangeList;
    QGCMAVLinkSystem*   _activeSystem           = nullptr;
    QTimer              _updateFrequencyTimer;
    QStringList         _systemNames;
    QmlObjectListModel  _systems;                           ///< List of QGCMAVLinkSystem
    QmlObjectListModel  _charts;                            ///< List of MAVLinkCharts
    QList<TimeScale_st*>_timeScaleSt;
    QList<Range_st*>    _rangeSt;

};

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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QPointF>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkMessageFieldLog)

class QGCMAVLinkMessage;
class MAVLinkChartController;
class QAbstractSeries;

//-----------------------------------------------------------------------------
/// MAVLink message field
class QGCMAVLinkMessageField : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE(<QtCharts/QAbstractSeries>)

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

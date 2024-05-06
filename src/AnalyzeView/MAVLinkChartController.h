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
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QVariantList>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkChartControllerLog)

class QGCMAVLinkMessageField;
class MAVLinkInspectorController;
class QAbstractSeries;

//-----------------------------------------------------------------------------
/// MAVLink message charting controller
class MAVLinkChartController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("MAVLinkInspectorController.h")
    Q_MOC_INCLUDE("MAVLinkMessageField.h")
    Q_MOC_INCLUDE(<QtCharts/qabstractseries.h>)

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

/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief MAVLink message inspector and charting controller
/// @author Gus Grubba <gus@auterion.com>

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkMessageFieldLog)

class QGCMAVLinkMessage;
class MAVLinkChartController;
class QAbstractSeries;

class QGCMAVLinkMessageField : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("QtCharts/QAbstractSeries")
    Q_PROPERTY(QString          name        READ    name        CONSTANT)
    Q_PROPERTY(QString          label       READ    label       CONSTANT)
    Q_PROPERTY(QString          type        READ    type        CONSTANT)
    Q_PROPERTY(QString          value       READ    value       NOTIFY valueChanged)
    Q_PROPERTY(bool             selectable  READ    selectable  NOTIFY selectableChanged)
    Q_PROPERTY(int              chartIndex  READ    chartIndex  CONSTANT)
    Q_PROPERTY(QAbstractSeries  *series     READ    series      NOTIFY seriesChanged)

public:
    QGCMAVLinkMessageField(QGCMAVLinkMessage *parent, const QString &name, const QString &type);
    ~QGCMAVLinkMessageField();

    QString name() const { return _name; }
    QString label();
    QString type() const { return _type;  }
    QString value() const { return _value; }
    bool selectable() const { return _selectable; }
    int chartIndex();

    bool selected() const { return (_pSeries != nullptr); }
    qreal rangeMin() const { return _rangeMin; }
    qreal rangeMax() const { return _rangeMax; }
    QAbstractSeries *series() { return _pSeries; }
    QList<QPointF> *values() { return &_values;}

    void setSelectable(bool sel);
    void updateValue(const QString &newValue, qreal val);
    void addSeries(MAVLinkChartController *chart, QAbstractSeries *series);
    void delSeries();
    void updateSeries();

signals:
    void seriesChanged();
    void selectableChanged();
    void valueChanged();

private:
    QString _type;
    QString _name;
    QGCMAVLinkMessage *_msg = nullptr;

    QString _value;
    bool _selectable = true;
    int _dataIndex  = 0;
    qreal _rangeMin = 0.;
    qreal _rangeMax = 0.;
    QList<QPointF> _values;

    QAbstractSeries *_pSeries = nullptr;
    MAVLinkChartController *_chart = nullptr;

    static constexpr int kDataLimit = (50 * 60);    ///< Arbitrary limit of 1 minute of data at 50Hz for now
};

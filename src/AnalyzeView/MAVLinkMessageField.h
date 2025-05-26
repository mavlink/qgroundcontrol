/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
    // QML_ELEMENT
    Q_MOC_INCLUDE(<QtCharts/QAbstractSeries>)
    Q_PROPERTY(QString                  name        READ name       CONSTANT)
    Q_PROPERTY(QString                  label       READ label      CONSTANT)
    Q_PROPERTY(QString                  type        READ type       CONSTANT)
    Q_PROPERTY(QString                  value       READ value      NOTIFY valueChanged)
    Q_PROPERTY(bool                     selectable  READ selectable NOTIFY selectableChanged)
    Q_PROPERTY(int                      chartIndex  READ chartIndex CONSTANT)
    Q_PROPERTY(const QAbstractSeries    *series     READ series     NOTIFY seriesChanged)

public:
    QGCMAVLinkMessageField(const QString &name, const QString &type, QGCMAVLinkMessage *parent = nullptr);
    ~QGCMAVLinkMessageField();

    QString name() const { return _name;  }
    QString label() const;
    QString type() const { return _type;  }
    QString value() const { return _value; }
    bool selectable() const { return _selectable; }
    bool selected() const { return !!_pSeries; }
    const QAbstractSeries *series() const { return _pSeries; }
    const QList<QPointF> *values() const { return &_values; }
    qreal rangeMin() const { return _rangeMin; }
    qreal rangeMax() const { return _rangeMax; }
    int chartIndex() const;

    void setSelectable(bool sel);
    void updateValue(const QString &newValue, qreal v);

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
    int _dataIndex = 0;
    qreal _rangeMin = 0;
    qreal _rangeMax = 0;
    QList<QPointF> _values;

    QAbstractSeries *_pSeries = nullptr;
    MAVLinkChartController *_chart = nullptr;
};

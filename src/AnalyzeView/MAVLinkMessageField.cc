/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkMessageField.h"
#include "MAVLinkChartController.h"
#include "MAVLinkMessage.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"

#include <QtCharts/QLineSeries>
#include <QtCharts/QAbstractSeries>

QGC_LOGGING_CATEGORY(MAVLinkMessageFieldLog, "qgc.analyzeview.mavlinkmessagefield")

QGCMAVLinkMessageField::QGCMAVLinkMessageField(QGCMAVLinkMessage *parent, const QString &name, const QString &type)
    : QObject(parent)
    , _type(type)
    , _name(name)
    , _msg(parent)
{
    // qCDebug(MAVLinkMessageFieldLog) << Q_FUNC_INFO << this;
    qCDebug(MAVLinkMessageFieldLog) << "Field:" << name << type;
}

QGCMAVLinkMessageField::~QGCMAVLinkMessageField()
{
    // qCDebug(MAVLinkMessageFieldLog) << Q_FUNC_INFO << this;
}

void QGCMAVLinkMessageField::addSeries(MAVLinkChartController *chart, QAbstractSeries *series)
{
    if (_pSeries) {
        return;
    }

    _dataIndex = 0;
    _chart = chart;
    _pSeries = series;
    emit seriesChanged();

    _msg->updateFieldSelection();
}

void QGCMAVLinkMessageField::delSeries()
{
    if (!_pSeries) {
        return;
    }

    _values.clear();
    QLineSeries *const lineSeries = static_cast<QLineSeries*>(_pSeries);
    (void) lineSeries->replace(_values);
    _chart = nullptr;
    _pSeries = nullptr;
    emit seriesChanged();

    _msg->updateFieldSelection();
}

QString QGCMAVLinkMessageField::label()
{
    return (_msg->name() + QStringLiteral(": ") + _name);
}

void QGCMAVLinkMessageField::setSelectable(bool sel)
{
    if (_selectable != sel) {
        _selectable = sel;
        emit selectableChanged();
    }
}

int QGCMAVLinkMessageField::chartIndex()
{
    return (_chart ? _chart->chartIndex() : 0);
}

void QGCMAVLinkMessageField::updateValue(const QString &newValue, qreal val)
{
    if (_value != newValue) {
        _value = newValue;
        emit valueChanged();
    }

    if (!_pSeries || !_chart) {
        return;
    }

    const int count = _values.count();
    if (count < kDataLimit) {
        QPointF point(QGC::bootTimeMilliseconds(), val);
        (void) _values.append(point);
    } else {
        if (_dataIndex >= count) {
            _dataIndex = 0;
        }

        _values[_dataIndex].setX(QGC::bootTimeMilliseconds());
        _values[_dataIndex].setY(val);
        _dataIndex++;
    }

    //-- Auto Range
    if (_chart->rangeYIndex() != 0) {
        return;
    }

    qreal vmin = std::numeric_limits<qreal>::max();
    qreal vmax = std::numeric_limits<qreal>::min();
    for (const QPointF point : _values) {
        const qreal val = point.y();
        if (vmax < val) {
            vmax = val;
        }
        if (vmin > val) {
            vmin = val;
        }
    }

    bool changed = false;
    if (qAbs(_rangeMin - vmin) > 0.000001) {
        _rangeMin = vmin;
        changed = true;
    }

    if (qAbs(_rangeMax - vmax) > 0.000001) {
        _rangeMax = vmax;
        changed = true;
    }

    if (changed) {
        _chart->updateYRange();
    }
}

void QGCMAVLinkMessageField::updateSeries()
{
    const int count = _values.count();
    if (count <= 1) {
        return;
    }

    QList<QPointF> points;
    points.reserve(count);
    int idx = _dataIndex;
    for (int i = 0; i < count; i++, idx++) {
        if (idx >= count) {
            idx = 0;
        }

        QPointF point(_values[idx]);
        (void) points.append(point);
    }

    QLineSeries *const lineSeries = static_cast<QLineSeries*>(_pSeries);
    lineSeries->replace(points);
}

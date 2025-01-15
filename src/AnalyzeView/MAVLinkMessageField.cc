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
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCharts/QLineSeries>
#include <QtCharts/QAbstractSeries>

QGC_LOGGING_CATEGORY(MAVLinkMessageFieldLog, "qgc.analyzeview.mavlinkmessagefield")

QGCMAVLinkMessageField::QGCMAVLinkMessageField(const QString &name, const QString &type, QGCMAVLinkMessage *parent)
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

    _chart = chart;
    _pSeries = series;
    emit seriesChanged();

    _dataIndex = 0;
    _msg->updateFieldSelection();
}

void QGCMAVLinkMessageField::delSeries()
{
    if (!_pSeries) {
        return;
    }

    _values.clear();
    QLineSeries *const lineSeries = static_cast<QLineSeries*>(_pSeries);
    lineSeries->replace(_values);
    _pSeries = nullptr;
    _chart = nullptr;
    emit seriesChanged();
    _msg->updateFieldSelection();
}

QString QGCMAVLinkMessageField::label() const
{
    return (_msg->name() + ": " + _name);
}

void QGCMAVLinkMessageField::setSelectable(bool sel)
{
    if (_selectable != sel) {
        _selectable = sel;
        emit selectableChanged();
    }
}

int QGCMAVLinkMessageField::chartIndex() const
{
    if (_chart) {
        return _chart->chartIndex();
    }

    return 0;
}

void QGCMAVLinkMessageField::updateValue(const QString &newValue, qreal v)
{
    if (_value != newValue) {
        _value = newValue;
        emit valueChanged();
    }

    if (!_pSeries || !_chart) {
        return;
    }

    const int count = _values.count();
    if (count < (50 * 60)) { ///< Arbitrary limit of 1 minute of data at 50Hz for now
        const QPointF p(qgcApp()->msecsSinceBoot(), v);
        _values.append(p);
    } else {
        if (_dataIndex >= count) {
            _dataIndex = 0;
        }
        _values[_dataIndex].setX(qgcApp()->msecsSinceBoot());
        _values[_dataIndex].setY(v);
        _dataIndex++;
    }

    if (_chart->rangeYIndex() != 0) {
        return;
    }

    qreal vmin = std::numeric_limits<qreal>::max();
    qreal vmax = std::numeric_limits<qreal>::min();
    for (const QPointF &point : _values) {
        const qreal v = point.y();
        if (vmax < v) {
            vmax = v;
        }
        if (vmin > v) {
            vmin = v;
        }
    }

    bool changed = false;
    if (std::abs(_rangeMin - vmin) > 0.000001) {
        _rangeMin = vmin;
        changed = true;
    }

    if (std::abs(_rangeMax - vmax) > 0.000001) {
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

    QList<QPointF> s;
    s.reserve(count);
    int idx = _dataIndex;
    for (int i = 0; i < count; i++, idx++) {
        if (idx >= count) {
            idx = 0;
        }

        const QPointF p(_values[idx]);
        s.append(p);
    }

    QLineSeries *const lineSeries = static_cast<QLineSeries*>(_pSeries);
    lineSeries->replace(s);
}

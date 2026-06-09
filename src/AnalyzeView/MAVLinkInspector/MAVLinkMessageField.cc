#include "MAVLinkMessageField.h"
#include "MAVLinkChartController.h"
#include "MAVLinkMessage.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtGraphs/QLineSeries>
#include <QtGraphs/QAbstractSeries>

#include <algorithm>
#include <cmath>

static constexpr qreal kMinDelta = 1e-6;

QGC_LOGGING_CATEGORY(MAVLinkMessageFieldLog, "AnalyzeView.MAVLinkMessageField")

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

void QGCMAVLinkMessageField::addSeries(MAVLinkChartController *chartController, QAbstractSeries *series)
{
    if (_pSeries) {
        return;
    }

    _chartController = chartController;
    _pSeries = series;
    emit seriesChanged();

    _dataIndex = 0;
    _bucketCount = std::max(1, chartController->plotPixelWidth());
    _bucketWidthMs = chartController->rangeXMs() / _bucketCount;
    _currentBucketStart = -1;
    _msg->updateFieldSelection();
}

void QGCMAVLinkMessageField::delSeries()
{
    if (!_pSeries) {
        return;
    }

    _values.clear();
    _rangeMin = std::numeric_limits<qreal>::max();
    _rangeMax = std::numeric_limits<qreal>::lowest();
    QLineSeries *const lineSeries = static_cast<QLineSeries*>(_pSeries);
    lineSeries->replace(_values);
    _pSeries = nullptr;
    _chartController = nullptr;
    _bucketCount = 0;
    _bucketWidthMs = 0;
    _currentBucketStart = -1;
    _dataIndex = 0;
    emit seriesChanged();
    _msg->updateFieldSelection();
}

void QGCMAVLinkMessageField::resetBucketing(int bucketCount, qreal bucketWidthMs)
{
    _bucketCount = std::max(1, bucketCount);
    _bucketWidthMs = bucketWidthMs;
    _currentBucketStart = -1;
    _currentBucketMin = 0;
    _currentBucketMax = 0;
    _dataIndex = 0;
    _values.clear();
    _rangeMin = std::numeric_limits<qreal>::max();
    _rangeMax = std::numeric_limits<qreal>::lowest();
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
    if (_chartController) {
        return _chartController->chartIndex();
    }

    return 0;
}

void QGCMAVLinkMessageField::updateValue(const QString &newValue, qreal v)
{
    if (_value != newValue) {
        _value = newValue;
        emit valueChanged();
    }

    if (!_pSeries || !_chartController || _bucketCount <= 0) {
        return;
    }

    const qreal now = qgcApp()->msecsSinceBoot();

    if (_currentBucketStart < 0) {
        // First sample — start first bucket
        _currentBucketStart = now;
        _currentBucketMin = v;
        _currentBucketMax = v;
        if (_chartController->rangeYIndex() == 0) {
            bool changed = false;
            if (std::abs(_rangeMin - v) > kMinDelta) {
                _rangeMin = v;
                changed = true;
            }
            if (std::abs(_rangeMax - v) > kMinDelta) {
                _rangeMax = v;
                changed = true;
            }
            if (changed) {
                _chartController->updateYRange();
            }
        }
        return;
    }

    if (now < _currentBucketStart + _bucketWidthMs) {
        // Still in current bucket — update min/max
        _currentBucketMin = std::min(_currentBucketMin, v);
        _currentBucketMax = std::max(_currentBucketMax, v);
        if (_chartController->rangeYIndex() == 0) {
            bool changed = false;
            if (v < _rangeMin - kMinDelta) {
                _rangeMin = v;
                changed = true;
            }
            if (v > _rangeMax + kMinDelta) {
                _rangeMax = v;
                changed = true;
            }
            if (changed) {
                _chartController->updateYRange();
            }
        }
        return;
    }

    // Crossed into next bucket — commit current bucket
    _commitBucket();

    // Start new bucket with this sample
    _currentBucketStart = now;
    _currentBucketMin = v;
    _currentBucketMax = v;

    // Update Y auto-range from committed data and current bucket
    if (_chartController->rangeYIndex() != 0) {
        return;
    }

    qreal vmin = _currentBucketMin;
    qreal vmax = _currentBucketMax;
    for (const QPointF &point : _values) {
        vmin = std::min(vmin, point.y());
        vmax = std::max(vmax, point.y());
    }

    bool changed = false;
    if (std::abs(_rangeMin - vmin) > kMinDelta) {
        _rangeMin = vmin;
        changed = true;
    }

    if (std::abs(_rangeMax - vmax) > kMinDelta) {
        _rangeMax = vmax;
        changed = true;
    }

    if (changed) {
        _chartController->updateYRange();
    }
}

void QGCMAVLinkMessageField::_commitBucket()
{
    const qreal bucketMidTime = _currentBucketStart + _bucketWidthMs * 0.5;
    const int maxPoints = _bucketCount * 2;

    // Append min and max points for this bucket (always two points to keep capacity predictable)
    const int count = _values.count();
    if (count < maxPoints) {
        _values.append(QPointF(bucketMidTime, _currentBucketMin));
        _values.append(QPointF(bucketMidTime, _currentBucketMax));
    } else {
        // Ring buffer wrap
        if (_dataIndex >= maxPoints) {
            _dataIndex = 0;
        }
        _values[_dataIndex] = QPointF(bucketMidTime, _currentBucketMin);
        _dataIndex++;
        if (_dataIndex >= maxPoints) {
            _dataIndex = 0;
        }
        _values[_dataIndex] = QPointF(bucketMidTime, _currentBucketMax);
        _dataIndex++;
    }
}

void QGCMAVLinkMessageField::updateSeries()
{
    const int count = _values.count();

    QList<QPointF> s;
    s.reserve(count + 2);
    int idx = _dataIndex;
    for (int i = 0; i < count; i++, idx++) {
        if (idx >= count) {
            idx = 0;
        }

        const QPointF p(_values[idx]);
        s.append(p);
    }

    // Append the in-progress bucket so the chart always shows the latest data
    if (_currentBucketStart >= 0) {
        const qreal now = qgcApp()->msecsSinceBoot();
        s.append(QPointF(now, _currentBucketMin));
        if (std::abs(_currentBucketMax - _currentBucketMin) > kMinDelta) {
            s.append(QPointF(now, _currentBucketMax));
        }
    }

    QLineSeries *const lineSeries = static_cast<QLineSeries*>(_pSeries);
    if (s.isEmpty()) {
        lineSeries->clear();
        return;
    }

    lineSeries->replace(s);
}

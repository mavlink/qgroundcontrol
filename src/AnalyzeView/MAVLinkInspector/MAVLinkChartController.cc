#include "MAVLinkChartController.h"
#include "MAVLinkInspectorController.h"
#include "MAVLinkMessageField.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtGraphs/QAbstractSeries>
#include <QtCore/QTimer>

static constexpr qreal kMinDelta = 1e-6;

QGC_LOGGING_CATEGORY(MAVLinkChartControllerLog, "AnalyzeView.MAVLinkChartController")

MAVLinkChartController::MAVLinkChartController(QObject *parent)
    : QObject(parent)
    , _updateSeriesTimer(new QTimer(this))
{
    // qCDebug(MAVLinkChartControllerLog) << Q_FUNC_INFO << this;

    (void) connect(_updateSeriesTimer, &QTimer::timeout, this, &MAVLinkChartController::_refreshSeries);
}

MAVLinkChartController::~MAVLinkChartController()
{
    // qCDebug(MAVLinkChartControllerLog) << Q_FUNC_INFO << this;
}

void MAVLinkChartController::setInspectorController(MAVLinkInspectorController *controller)
{
    if (_inspectorController == controller) {
        return;
    }

    _inspectorController = controller;
    updateXRange();
}

void MAVLinkChartController::setRangeYIndex(quint32 index)
{
    if (index == _rangeYIndex) {
        return;
    }

    if (!_inspectorController || index >= static_cast<quint32>(_inspectorController->rangeSt().count())) {
        return;
    }

    _rangeYIndex = index;
    emit rangeYIndexChanged();

    // If not Auto, use defined range
    const qreal range = _inspectorController->rangeSt()[static_cast<int>(index)]->range;
    if (_rangeYIndex > 0) {
        _rangeYMin = -range;
        emit rangeYMinChanged();

        _rangeYMax = range;
        emit rangeYMaxChanged();
    }
}

qreal MAVLinkChartController::rangeXMs() const
{
    if (!_inspectorController) {
        return 5000.0;
    }
    if (_rangeXIndex < static_cast<quint32>(_inspectorController->timeScaleSt().count())) {
        return static_cast<qreal>(_inspectorController->timeScaleSt()[static_cast<int>(_rangeXIndex)]->timeScale);
    }
    return 5000.0;
}

void MAVLinkChartController::setRangeXIndex(quint32 index)
{
    if (index == _rangeXIndex) {
        return;
    }

    _rangeXIndex = index;
    emit rangeXIndexChanged();

    updateXRange();
    _resetFieldBucketing();
}

void MAVLinkChartController::updateXRange()
{
    if (!_inspectorController) {
        return;
    }

    if (_rangeXIndex >= static_cast<quint32>(_inspectorController->timeScaleSt().count())) {
        return;
    }

    const qint64 bootTime = static_cast<qint64>(qgcApp()->msecsSinceBoot());
    _rangeXMax = static_cast<qreal>(bootTime);
    emit rangeXMaxChanged();

    _rangeXMin = static_cast<qreal>(bootTime - _inspectorController->timeScaleSt()[static_cast<int>(_rangeXIndex)]->timeScale);
    emit rangeXMinChanged();
}

void MAVLinkChartController::updateYRange()
{
    if (_chartFields.isEmpty()) {
        return;
    }

    qreal vmin = std::numeric_limits<qreal>::max();
    qreal vmax = std::numeric_limits<qreal>::lowest();
    for (const QVariant &field : _chartFields) {
        QObject *const object = qvariant_cast<QObject*>(field);
        QGCMAVLinkMessageField *const pField = qobject_cast<QGCMAVLinkMessageField*>(object);
        if (pField) {
            vmin = std::min(vmin, pField->rangeMin());
            vmax = std::max(vmax, pField->rangeMax());
        }
    }

    if (vmin > vmax) {
        return; // No field has received data yet (sentinel values)
    }

    if (qAbs(vmax - vmin) < kMinDelta) {
        vmin -= 1.0;
        vmax += 1.0;
    } else {
        const qreal padding = (vmax - vmin) * 0.05;
        vmin -= padding;
        vmax += padding;
    }

    if (qAbs(_rangeYMin - vmin) > kMinDelta) {
        _rangeYMin = vmin;
        emit rangeYMinChanged();
    }

    if (qAbs(_rangeYMax - vmax) > kMinDelta) {
        _rangeYMax = vmax;
        emit rangeYMaxChanged();
    }
}

void MAVLinkChartController::_refreshSeries()
{
    updateXRange();

    for (QVariant &field : _chartFields) {
        QObject *const object = qvariant_cast<QObject*>(field);
        QGCMAVLinkMessageField *const pField = qobject_cast<QGCMAVLinkMessageField*>(object);
        if(pField) {
            pField->updateSeries();
        }
    }

    if (_rangeYIndex == 0) {
        updateYRange();
    }
}

void MAVLinkChartController::addSeries(QGCMAVLinkMessageField *field, QAbstractSeries *series)
{
    if (!field || !series) {
        return;
    }

    const QVariant f = QVariant::fromValue(field);
    for (const QVariant &chartField : _chartFields) {
        if (chartField == f) {
            return;
        }
    }

    _chartFields.append(f);
    field->addSeries(this, series);
    emit chartFieldsChanged();

    _updateSeriesTimer->start(kUpdateFrequency);
}

void MAVLinkChartController::delSeries(QGCMAVLinkMessageField *field)
{
    if (!field) {
        return;
    }

    field->delSeries();

    const QVariant f = QVariant::fromValue(field);
    const QList<QVariant>::const_iterator it = std::find(_chartFields.constBegin(), _chartFields.constEnd(), f);
    if (it != _chartFields.constEnd()) {
        (void) _chartFields.erase(it);
        emit chartFieldsChanged();

        if (_chartFields.isEmpty()) {
            updateXRange();
            _updateSeriesTimer->stop();
        }
    }
}

void MAVLinkChartController::setPlotPixelWidth(int width)
{
    if (width == _plotPixelWidth) {
        return;
    }

    _plotPixelWidth = width;
    emit plotPixelWidthChanged();
    _resetFieldBucketing();
}

void MAVLinkChartController::_resetFieldBucketing()
{
    if (_plotPixelWidth <= 0) {
        return;
    }

    const qreal bucketWidthMs = rangeXMs() / _plotPixelWidth;
    for (const QVariant &field : _chartFields) {
        QObject *const object = qvariant_cast<QObject*>(field);
        QGCMAVLinkMessageField *const pField = qobject_cast<QGCMAVLinkMessageField*>(object);
        if (pField) {
            pField->resetBucketing(_plotPixelWidth, bucketWidthMs);
        }
    }
}

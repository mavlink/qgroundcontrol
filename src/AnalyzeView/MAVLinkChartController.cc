/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkChartController.h"
#include "MAVLinkInspectorController.h"
#include "MAVLinkMessageField.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCharts/QAbstractSeries>
#include <QtCore/QTimer>

Q_DECLARE_METATYPE(QAbstractSeries*)

QGC_LOGGING_CATEGORY(MAVLinkChartControllerLog, "qgc.analyzeview.mavlinkchartcontroller")

MAVLinkChartController::MAVLinkChartController(MAVLinkInspectorController *controller, int index, QObject *parent)
    : QObject(parent)
    , _index(index)
    , _controller(controller)
    , _updateSeriesTimer(new QTimer(this))
{
    // qCDebug(MAVLinkChartControllerLog) << Q_FUNC_INFO << this;

    (void) qRegisterMetaType<QAbstractSeries*>("QAbstractSeries*");

    (void) connect(_updateSeriesTimer, &QTimer::timeout, this, &MAVLinkChartController::_refreshSeries);

    updateXRange();
}

MAVLinkChartController::~MAVLinkChartController()
{
    // qCDebug(MAVLinkChartControllerLog) << Q_FUNC_INFO << this;
}

void MAVLinkChartController::setRangeYIndex(quint32 index)
{
    if (index == _rangeYIndex) {
        return;
    }

    if (index >= static_cast<quint32>(_controller->rangeSt().count())) {
        return;
    }

    _rangeYIndex = index;
    emit rangeYIndexChanged();

    // If not Auto, use defined range
    const qreal range = _controller->rangeSt()[static_cast<int>(index)]->range;
    if (_rangeYIndex > 0) {
        _rangeYMin = -range;
        emit rangeYMinChanged();

        _rangeYMax = range;
        emit rangeYMaxChanged();
    }
}

void MAVLinkChartController::setRangeXIndex(quint32 index)
{
    if (index == _rangeXIndex) {
        return;
    }

    _rangeXIndex = index;
    emit rangeXIndexChanged();

    updateXRange();
}

void MAVLinkChartController::updateXRange()
{
    if (_rangeXIndex >= static_cast<quint32>(_controller->timeScaleSt().count())) {
        return;
    }

    const qint64 bootTime = static_cast<qint64>(qgcApp()->msecsSinceBoot());
    _rangeXMax = QDateTime::fromMSecsSinceEpoch(bootTime);
    emit rangeXMaxChanged();

    _rangeXMin = QDateTime::fromMSecsSinceEpoch(bootTime - _controller->timeScaleSt()[static_cast<int>(_rangeXIndex)]->timeScale);
    emit rangeXMinChanged();
}

void MAVLinkChartController::updateYRange()
{
    if (_chartFields.isEmpty()) {
        return;
    }

    qreal vmin = std::numeric_limits<qreal>::max();
    qreal vmax = std::numeric_limits<qreal>::min();
    for (const QVariant &field : _chartFields) {
        QObject *const object = qvariant_cast<QObject*>(field);
        QGCMAVLinkMessageField *const pField = qobject_cast<QGCMAVLinkMessageField*>(object);
        if (pField) {
            if (vmax < pField->rangeMax()) {
                vmax = pField->rangeMax();
            }

            if (vmin > pField->rangeMin()) {
                vmin = pField->rangeMin();
            }
        }
    }

    if (qAbs(_rangeYMin - vmin) > 0.000001) {
        _rangeYMin = vmin;
        emit rangeYMinChanged();
    }

    if (qAbs(_rangeYMax - vmax) > 0.000001) {
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

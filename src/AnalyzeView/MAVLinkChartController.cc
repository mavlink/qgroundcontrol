/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkChartController.h"
#include "MAVLinkInspectorController.h"
#include "QGC.h"
#include "MAVLinkMessageField.h"
#include "QGCLoggingCategory.h"

#include <QtCharts/QAbstractSeries>

QGC_LOGGING_CATEGORY(MAVLinkChartControllerLog, "qgc.analyzeview.mavlinkchartcontroller")

Q_DECLARE_METATYPE(QAbstractSeries*)

#define UPDATE_FREQUENCY (1000 / 15)    // 15Hz

//-----------------------------------------------------------------------------
MAVLinkChartController::MAVLinkChartController(MAVLinkInspectorController *parent, int index)
    : QObject(parent)
    , _index(index)
    , _controller(parent)
{
    connect(&_updateSeriesTimer, &QTimer::timeout, this, &MAVLinkChartController::_refreshSeries);
    updateXRange();
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::setRangeYIndex(quint32 r)
{
    if(r < static_cast<quint32>(_controller->rangeSt().count())) {
        _rangeYIndex = r;
        qreal range = _controller->rangeSt()[static_cast<int>(r)]->range;
        emit rangeYIndexChanged();
        //-- If not Auto, use defined range
        if(_rangeYIndex > 0) {
            _rangeYMin = -range;
            emit rangeYMinChanged();
            _rangeYMax = range;
            emit rangeYMaxChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::setRangeXIndex(quint32 t)
{
    _rangeXIndex = t;
    emit rangeXIndexChanged();
    updateXRange();
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::updateXRange()
{
    if(_rangeXIndex < static_cast<quint32>(_controller->timeScaleSt().count())) {
        qint64 t = static_cast<qint64>(QGC::bootTimeMilliseconds());
        _rangeXMax = QDateTime::fromMSecsSinceEpoch(t);
        _rangeXMin = QDateTime::fromMSecsSinceEpoch(t - _controller->timeScaleSt()[static_cast<int>(_rangeXIndex)]->timeScale);
        emit rangeXMinChanged();
        emit rangeXMaxChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::updateYRange()
{
    if(_chartFields.count()) {
        qreal vmin  = std::numeric_limits<qreal>::max();
        qreal vmax  = std::numeric_limits<qreal>::min();
        for(int i = 0; i < _chartFields.count(); i++) {
            QObject* object = qvariant_cast<QObject*>(_chartFields.at(i));
            QGCMAVLinkMessageField* pField = qobject_cast<QGCMAVLinkMessageField*>(object);
            if(pField) {
                if(vmax < pField->rangeMax()) vmax = pField->rangeMax();
                if(vmin > pField->rangeMin()) vmin = pField->rangeMin();
            }
        }
        if(std::abs(_rangeYMin - vmin) > 0.000001) {
            _rangeYMin = vmin;
            emit rangeYMinChanged();
        }
        if(std::abs(_rangeYMax - vmax) > 0.000001) {
            _rangeYMax = vmax;
            emit rangeYMaxChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::_refreshSeries()
{
    updateXRange();
    for(int i = 0; i < _chartFields.count(); i++) {
        QObject* object = qvariant_cast<QObject*>(_chartFields.at(i));
        QGCMAVLinkMessageField* pField = qobject_cast<QGCMAVLinkMessageField*>(object);
        if(pField) {
            pField->updateSeries();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::addSeries(QGCMAVLinkMessageField* field, QAbstractSeries* series)
{
    if(field && series) {
        QVariant f = QVariant::fromValue(field);
        for(int i = 0; i < _chartFields.count(); i++) {
            if(_chartFields.at(i) == f) {
                return;
            }
        }
        _chartFields.append(f);
        field->addSeries(this, series);
        emit chartFieldsChanged();
        _updateSeriesTimer.start(UPDATE_FREQUENCY);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::delSeries(QGCMAVLinkMessageField* field)
{
    if(field) {
        field->delSeries();
        QVariant f = QVariant::fromValue(field);
        for(int i = 0; i < _chartFields.count(); i++) {
            if(_chartFields.at(i) == f) {
                _chartFields.removeAt(i);
                emit chartFieldsChanged();
                if(_chartFields.count() == 0) {
                    updateXRange();
                    _updateSeriesTimer.stop();
                }
                return;
            }
        }
    }
}

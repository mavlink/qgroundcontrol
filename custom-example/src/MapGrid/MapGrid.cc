/****************************************************************************
 *
 *   (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapGrid.h"

//-----------------------------------------------------------------------------
MapGrid::MapGrid()
    : QObject()
{
    _mapGridMGRS = new MapGridMGRS();
    _mapGridThread = new QThread(this);
    _mapGridThread->setObjectName("MapGrid");
    _mapGridMGRS->moveToThread(_mapGridThread);
    connect(_mapGridThread, &QThread::finished, _mapGridMGRS, &QObject::deleteLater, Qt::QueuedConnection);
    connect(this, &MapGrid::geometryChangedSignal, _mapGridMGRS, &MapGridMGRS::geometryChanged, Qt::QueuedConnection);
    connect(_mapGridMGRS, &MapGridMGRS::updateValues, this, &MapGrid::updateValues, Qt::QueuedConnection);
    _mapGridThread->start();
}

//-----------------------------------------------------------------------------
void
MapGrid::updateValues(const QVariant& newValues)
{
    _calculationRunning = false;
    if (newValues.userType() == QMetaType::Bool) {
        return;
    }
    _values = newValues;
    emit valuesChanged();
    if (_calculationPending) {
        _calculationPending = false;
        geometryChanged(_pendingZoomLevel, _pendingTopLeft, _pendingBottomRight);
    }
}

//-----------------------------------------------------------------------------
void
MapGrid::geometryChanged(double zoomLevel, const QGeoCoordinate& topLeft, const QGeoCoordinate& bottomRight)
{
    if (!_calculationRunning) {
        _calculationRunning = true;
        emit geometryChangedSignal(zoomLevel, topLeft, bottomRight);
    } else {
        _calculationPending = true;
        _pendingZoomLevel = zoomLevel;
        _pendingTopLeft = topLeft;
        _pendingBottomRight = bottomRight;
    }
}

//-----------------------------------------------------------------------------

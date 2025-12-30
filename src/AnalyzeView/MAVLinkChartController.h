/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkChartControllerLog)

class MAVLinkInspectorController;
class QAbstractSeries;
class QGCMAVLinkMessageField;
class QTimer;

class MAVLinkChartController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("MAVLinkInspectorController.h")
    Q_MOC_INCLUDE("MAVLinkMessageField.h")
    Q_MOC_INCLUDE("QtCharts/qabstractseries.h")

    Q_PROPERTY(MAVLinkInspectorController*  inspectorController READ inspectorController WRITE setInspectorController REQUIRED)
    Q_PROPERTY(int          chartIndex  MEMBER _chartIndex                          REQUIRED)
    Q_PROPERTY(QVariantList chartFields READ chartFields                            NOTIFY chartFieldsChanged)
    Q_PROPERTY(QDateTime    rangeXMin   READ rangeXMin                              NOTIFY rangeXMinChanged)
    Q_PROPERTY(QDateTime    rangeXMax   READ rangeXMax                              NOTIFY rangeXMaxChanged)
    Q_PROPERTY(qreal        rangeYMin   READ rangeYMin                              NOTIFY rangeYMinChanged)
    Q_PROPERTY(qreal        rangeYMax   READ rangeYMax                              NOTIFY rangeYMaxChanged)
    Q_PROPERTY(quint32      rangeYIndex READ rangeYIndex    WRITE setRangeYIndex    NOTIFY rangeYIndexChanged)
    Q_PROPERTY(quint32      rangeXIndex READ rangeXIndex    WRITE setRangeXIndex    NOTIFY rangeXIndexChanged)


public:
    explicit MAVLinkChartController(QObject *parent = nullptr);
    ~MAVLinkChartController();

    Q_INVOKABLE void addSeries(QGCMAVLinkMessageField *field, QAbstractSeries *series);
    Q_INVOKABLE void delSeries(QGCMAVLinkMessageField *field);

    void setInspectorController(MAVLinkInspectorController *inspectorController);
    MAVLinkInspectorController *inspectorController() const { return _inspectorController; }
    QVariantList chartFields() const { return _chartFields; }
    QDateTime rangeXMin() const { return _rangeXMin; }
    QDateTime rangeXMax() const { return _rangeXMax; }
    qreal rangeYMin() const { return _rangeYMin; }
    qreal rangeYMax() const { return _rangeYMax; }
    quint32 rangeXIndex() const { return _rangeXIndex; }
    quint32 rangeYIndex() const { return _rangeYIndex; }
    int chartIndex() const { return _chartIndex; }

    void setRangeXIndex(quint32 index);
    void setRangeYIndex(quint32 index);
    void updateXRange();
    void updateYRange();

signals:
    void chartFieldsChanged();
    void rangeXMinChanged();
    void rangeXMaxChanged();
    void rangeYMinChanged();
    void rangeYMaxChanged();
    void rangeYIndexChanged();
    void rangeXIndexChanged();

private slots:
    void _refreshSeries();

private:
    int _chartIndex = 0;
    MAVLinkInspectorController *_inspectorController = nullptr;
    QTimer *_updateSeriesTimer = nullptr;

    QDateTime _rangeXMin;
    QDateTime _rangeXMax;
    qreal _rangeYMin = 0;
    qreal _rangeYMax = 1;
    quint32 _rangeXIndex = 0;   ///< 5 Seconds
    quint32 _rangeYIndex = 0;   ///< Auto Range
    QVariantList _chartFields;

    static constexpr int kUpdateFrequency = 1000 / 15;  ///< 15Hz
};

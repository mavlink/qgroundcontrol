#ifndef CURVECALIBRATOR_H
#define CURVECALIBRATOR_H

#include <QWidget>
#include <QVector>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPen>
#include <QColor>
#include <QString>
#include <QSignalMapper>
#include <QDebug>

#include "AbstractCalibrator.h"

class CurveCalibrator : public AbstractCalibrator
{
Q_OBJECT
public:
    explicit CurveCalibrator(QString title = QString(), QWidget *parent = 0);
    ~CurveCalibrator();

    void set(const QVector<float> &data);
signals:
    void setpointChanged(int setpoint, float raw);    

protected slots:
    void setSetpoint(int setpoint);

protected:
    QVector<double> *setpoints;
    QVector<double> *positions;
    QwtPlot *plot;
    QwtPlotCurve *curve;

    QSignalMapper *signalMapper;
};

#endif // CURVECALIBRATOR_H

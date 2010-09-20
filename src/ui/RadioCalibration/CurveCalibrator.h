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

#include "AbstractCalibrator.h"

class CurveCalibrator : public AbstractCalibrator
{
Q_OBJECT
public:
    explicit CurveCalibrator(QString title = QString(), QWidget *parent = 0);

signals:
    void setpointChanged(float[5]);

protected:
    QVector<double> setpoints;
    QVector<double> positions;
    QwtPlot *plot;
    QwtPlotCurve *curve;
};

#endif // CURVECALIBRATOR_H

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

class CurveCalibrator : public QWidget
{
Q_OBJECT
public:
    explicit CurveCalibrator(QString title = QString(), QWidget *parent = 0);

signals:
    void setpointChanged(float[5]);
public slots:
    void channelChanged(float raw);

protected:
    QVector<double> setpoints;
    QVector<double> positions;
    QwtPlot *plot;
    QwtPlotCurve *curve;
    QLabel *pulseWidth;
};

#endif // CURVECALIBRATOR_H

#include "CurveCalibrator.h"

CurveCalibrator::CurveCalibrator(QString titleString, QWidget *parent) :
    QWidget(parent),
    setpoints(QVector<double>(5)),
    positions(QVector<double>())

{
    QGridLayout *grid = new QGridLayout(this);
    QLabel *title = new QLabel(titleString);
    grid->addWidget(title, 0, 0, 1, 5, Qt::AlignHCenter);

    QLabel *pulseWidthTitle = new QLabel(tr("Pulse Width (us)"));
    pulseWidth = new QLabel();
    QHBoxLayout *pulseLayout = new QHBoxLayout();
    pulseLayout->addWidget(pulseWidthTitle);
    pulseLayout->addWidget(pulseWidth);
    grid->addLayout(pulseLayout, 1, 0, 1, 5, Qt::AlignHCenter);

    for (int i=0; i<=100; i=i+100/4)
        positions.append(static_cast<double>(i));


    setpoints.fill(1500);

    plot = new QwtPlot();

    grid->addWidget(plot, 2, 0, 1, 5, Qt::AlignHCenter);


    plot->setAxisScale(QwtPlot::yLeft, 1000, 2000, 200);

    curve = new QwtPlotCurve();
    curve->setData(positions, setpoints);
    curve->attach(plot);

    plot->replot();


    this->setLayout(grid);
}

void CurveCalibrator::channelChanged(float raw)
{
    pulseWidth->setText(QString::number(static_cast<double>(raw)));
}

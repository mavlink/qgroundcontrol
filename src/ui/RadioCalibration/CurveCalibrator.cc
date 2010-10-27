#include "CurveCalibrator.h"

CurveCalibrator::CurveCalibrator(QString titleString, QWidget *parent) :
    AbstractCalibrator(parent),
    setpoints(new QVector<double>(5)),
    positions(new QVector<double>())
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
        positions->append(static_cast<double>(i));


    setpoints->fill(1500);

    plot = new QwtPlot();

    grid->addWidget(plot, 2, 0, 1, 5, Qt::AlignHCenter);


    plot->setAxisScale(QwtPlot::yLeft, 1000, 2000, 200);
    plot->setAxisScale(QwtPlot::xBottom, 0, 100, 25);

    curve = new QwtPlotCurve();
    curve->setPen(QPen(QColor(QString("lime"))));
    curve->setData(*positions, *setpoints);    
    curve->attach(plot);

    plot->replot();

    QPushButton *zero = new QPushButton(tr("0 %"));
    QPushButton *twentyfive = new QPushButton(tr("25 %"));
    QPushButton *fifty = new QPushButton(tr("50 %"));
    QPushButton *seventyfive = new QPushButton(tr("75 %"));
    QPushButton *hundred = new QPushButton(tr("100 %"));

    grid->addWidget(zero, 3, 0);
    grid->addWidget(twentyfive, 3, 1);
    grid->addWidget(fifty, 3, 2);
    grid->addWidget(seventyfive, 3, 3);
    grid->addWidget(hundred, 3, 4);

    this->setLayout(grid);

    signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(zero, 0);
    signalMapper->setMapping(twentyfive, 1);
    signalMapper->setMapping(fifty, 2);
    signalMapper->setMapping(seventyfive, 3);
    signalMapper->setMapping(hundred, 4);

    connect(zero, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(twentyfive, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(fifty, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(seventyfive, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(hundred, SIGNAL(clicked()), signalMapper, SLOT(map()));
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setSetpoint(int)));
}

CurveCalibrator::~CurveCalibrator()
{
    delete setpoints;
    delete positions;
}

void CurveCalibrator::setSetpoint(int setpoint)
{   
    if (setpoint == 0 || setpoint == 4)
    {
        setpoints->replace(setpoint, static_cast<double>(logExtrema()));
    }
    else
    {
        setpoints->replace(setpoint, static_cast<double>(logAverage()));
    }
    curve->setData(*positions, *setpoints);
    plot->replot();

    emit setpointChanged(setpoint, static_cast<float>(setpoints->value(setpoint)));
}

void CurveCalibrator::set(const QVector<float> &data)
{
    if (data.size() == 5)
    {
        for (int i=0; i<data.size(); ++i)
            setpoints->replace(i, static_cast<double>(data[i]));
        curve->setData(*positions, *setpoints);
        plot->replot();
    }
    else
    {
        qDebug() << __FILE__ << __LINE__ << ": wrong data vector size";
    }
}

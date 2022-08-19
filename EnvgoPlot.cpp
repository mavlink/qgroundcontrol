#include "EnvgoPlot.h"
#include "qcustomplot.h"
#include <QDebug>

EnvgoPlotClass::EnvgoPlotClass(QQuickItem *parent) :
    QQuickPaintedItem(parent),
    plot_area(nullptr)
{
    setFlag(QQuickItem::ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);

    connect(this, &QQuickPaintedItem::widthChanged,
            this, &EnvgoPlotClass::update_plot_size);

    connect(this, &QQuickPaintedItem::heightChanged,
            this, &EnvgoPlotClass::update_plot_size);
}


EnvgoPlotClass::~EnvgoPlotClass()
{
    delete plot_area;
}


void EnvgoPlotClass::init()
{
    plot_area = new QCustomPlot();

    update_plot_size();

    plot_area->addGraph();
    plot_area->graph(0)->setScatterStyle(QCPScatterStyle::ssNone);
    plot_area->graph(0)->setLineStyle(QCPGraph::lsLine);
    plot_area->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    combobox_idx = 0;
    qvs_idx = -1;

    QVector<double> qv;
    qv.append(2.5); // here!
    for(int i=0; i<10; i++)
    {
        qvs.append(qv);
    }

    // init done - plot empty graph
    // clear_data();
    // plot();
    combobox_idx = 5; // here!
    plot_clicked(); // here!
    qDebug() << "end of init";
}


void EnvgoPlotClass::paint(QPainter* painter)
{
    if (plot_area)
    {
        QPixmap picture(boundingRect().size().toSize());
        QCPPainter qcpPainter(&picture);

        plot_area->toPainter(&qcpPainter);

        painter->drawPixmap(QPoint(), picture);
    }
}


// data is a vector of size 10, where
//      data[0] = time
//      data[1] = average speed
//      data[2] = distance travelled
//      data[3] = remaining battery
//      data[4] = height above water
//      data[5] = temperature
//      data[6] = motor temperature
//      data[7] = motor controller temperature
//      data[8] = battery temperature
//      data[9] = servo temperature
void EnvgoPlotClass::add_data(QVector<double> data)
{
    for(unsigned int i=0; i<10; i++)
    {
        qvs[i].append(data[i]);
    }

    if(qvs_idx > 0)
    {
        plot_area->graph(0)->setData(qvs[0], qvs[qvs_idx]);
        plot();
    }
}


void EnvgoPlotClass::plot()
{
    plot_area->graph(0)->rescaleAxes(true);
    plot_area->replot();
    plot_area->update();
}


void EnvgoPlotClass::clear_data()
{
    QVector<double> empty;
    plot_area->graph(0)->setData(empty, empty);
}


void EnvgoPlotClass::plot_clicked()
{
    if(combobox_idx > 0)
    {
        plot_area->graph(0)->setData(qvs[0], qvs[combobox_idx]);
        qvs_idx = combobox_idx;
    }

    else
    {
        clear_data();
        qvs_idx = -1;
    }

    plot();
}



int EnvgoPlotClass::cb_idx() {
    return combobox_idx;
}

void EnvgoPlotClass::set_cb_idx(int index) {
    if(combobox_idx != index) {
        combobox_idx = index;
        emit cb_idx_changed();
    }
}


void EnvgoPlotClass::update_plot_size()
{
    if (plot_area)
    {
        plot_area->setGeometry(0, 0, 300, 300);
        plot_area->setViewport(QRect(0, 0, 300, 300));
    }
}    

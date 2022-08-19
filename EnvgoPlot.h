#ifndef ENVGOPLOT_H
#define ENVGOPLOT_H

#include <QtQuick>
#include <QVector>

class QCustomPlot;
class QCPAbstractPlottable;


class EnvgoPlotClass : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int cbIdx READ cbIdx WRITE setCbIdx NOTIFY cbIdxChanged)

public:
    EnvgoPlotClass(QQuickItem *parent = nullptr);
    virtual ~EnvgoPlotClass();
    void paint(QPainter* painter);
    Q_INVOKABLE void init();

    Q_INVOKABLE void plot_clicked();
    Q_INVOKABLE void add_data(QVector<double> data); // adds data and updates graph
    int cb_idx();

protected:
    void clear_data();
    void plot();

private:
    QCustomPlot *plot_area;
    int combobox_idx;
    int qvs_idx;
    QVector<QVector<double>> qvs;

signals:
    void cb_idx_changed();

public slots:
    void set_cb_idx(int idx);

private slots:
    void update_plot_size();    
};


#endif // ENVGOPLOT_H

#ifndef QGCXYPLOT_H
#define QGCXYPLOT_H

#include "QGCToolWidgetItem.h"

namespace Ui
{
class QGCXYPlot;
}

class UASInterface;
class QwtPlot;
class XYPlotCurve;

class QGCXYPlot : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCXYPlot(QWidget *parent = 0);
    ~QGCXYPlot();
    virtual void setEditMode(bool editMode);

public slots:
    void writeSettings(QSettings& settings);
    void readSettings(const QSettings& settings);
    void readSettings(const QString& pre,const QVariantMap& settings);
    void appendData(int uasId, const QString& curve, const QString& unit, const QVariant& variant, quint64 usec);
    void clearPlot();
    void styleChanged(bool styleIsDark);
    void updateMinMaxSettings();


private slots:
    void on_maxDataShowSpinBox_valueChanged(int value);
    void on_stopStartButton_toggled(bool checked);
    void setTimeAxis();
    void on_timeScrollBar_valueChanged(int value);

private:
    Ui::QGCXYPlot *ui;
    QwtPlot *plot;
    XYPlotCurve* xycurve;

    bool autoScaleTime;
    double x; /**< Last unused value for the x-coordinate */
    quint64 x_timestamp_us; /**< Timestamp that we last recieved a value for x */
    bool x_valid; /**< Whether we have recieved an x value but so far no corresponding y value */
    double y; /**< Last unused value for the x-coordinate */
    quint64 y_timestamp_us; /**< Timestamp that we last recieved a value for x */
    bool y_valid; /**< Whether we have recieved an x value but so far no corresponding y value */
    quint64 max_timestamp_diff_us; /**< Only combine x and y to a data point if the timestamp for both doesn't differ by more than this */
};

#endif // QGCXYPLOT_H

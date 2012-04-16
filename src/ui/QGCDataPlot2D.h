#ifndef QGCDATAPLOT2D_H
#define QGCDATAPLOT2D_H

#include <QWidget>
#include <QFile>
#include "IncrementalPlot.h"
#include "LogCompressor.h"

namespace Ui
{
class QGCDataPlot2D;
}

class QGCDataPlot2D : public QWidget
{
    Q_OBJECT
public:
    QGCDataPlot2D(QWidget *parent = 0);
    ~QGCDataPlot2D();

    /** @brief Calculate and display regression function*/
    bool calculateRegression(QString xName, QString yName, QString method="linear");

    /** @brief Linear regression over data points */
    bool linearRegression(double *x, double *y, int n, double *a, double *b, double *r);

public slots:
    /** @brief Load previously selected file */
    void loadFile();
    /** @brief Load file with this name */
    void loadFile(QString file);
    /** @brief Reload a file, with filtering enabled */
    void reloadFile();
    void selectFile();
    void loadCsvLog(QString file, QString xAxisName="", QString yAxisFilter="");
    void loadRawLog(QString file, QString xAxisName="", QString yAxisFilter="");
    void saveCsvLog();
    /** @brief Save plot to PDF or SVG */
    void savePlot();
    /** @brief Export PDF file */
    void exportPDF(QString fileName);
    /** @brief Export SVG file */
    void exportSVG(QString file);
    /** @brief Print or save PDF file (MacOS/Linux) */
    void print();
    /** @brief Calculate and display regression function*/
    bool calculateRegression();

signals:
    void visibilityChanged(bool visible);

protected:
    void showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        emit visibilityChanged(true);
    }

    void hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
        emit visibilityChanged(false);
    }

    void changeEvent(QEvent *e);
    IncrementalPlot* plot;
    LogCompressor* compressor;
    QFile* logFile;
    QString fileName;
    QStringList curveNames;

private:
    Ui::QGCDataPlot2D *ui;
};

#endif // QGCDATAPLOT2D_H

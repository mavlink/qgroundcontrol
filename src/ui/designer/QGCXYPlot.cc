#include <QDockWidget>

#include "QGCXYPlot.h"
#include "ui_QGCXYPlot.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"
#include "IncrementalPlot.h"
#include "QGCApplication.h"

#include <float.h>
#include <qwt_plot.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_engine.h>

class XYPlotCurve : public QwtPlotItem
{
public:
    XYPlotCurve() {
        m_maxStorePoints = 10000;
        m_maxShowPoints = 15;
        setItemAttribute(QwtPlotItem::AutoScale);
        minMaxSet = false;
        m_color = Qt::white;
        m_smoothPoints = 1;
        m_startIndex = -1; //Disable
    }

    void setMaxDataStorePoints(int max) { m_maxStorePoints = max; itemChanged(); }
    void setMaxDataShowPoints(int max) { m_maxShowPoints = max; itemChanged(); }
    void setSmoothPoints(int smoothPoints) { m_smoothPoints = smoothPoints; itemChanged(); }
    int maxDataStorePoints() const { return m_maxStorePoints; }
    int maxDataShowPoints() const { return m_maxShowPoints; }
    int smoothPoints() const { return m_smoothPoints; }

    /** Append data, returning the number of removed items */
    int appendData(const QPointF &data) {
        m_data.append(data);
        int removed = 0;
        while (m_data.size() > m_maxShowPoints) {
            ++removed;
            m_data.removeFirst();
        }
        if (!minMaxSet) {
            xmin = xmax = data.x();
            ymin = ymax = data.y();
            minMaxSet = true;
            previousTime =xmin;
        } else if (m_autoScale) {
            if (m_autoScaleTime) {
                xmin += removed * (data.x() - previousTime);
                previousTime = data.x();
            } else {
                xmin = qMin(qreal(xmin), data.x());
            }
            xmax = qMax(qreal(xmax), data.x());
            ymin = qMin(qreal(ymin), data.y());
            ymax = qMax(qreal(ymax), data.y());
        }
        itemChanged();
        return removed;
    }
    void clear() {
        minMaxSet = false;
        m_data.clear();
        itemChanged();
    }
    void setColor(const QColor &color) {
        m_color = color;
    }
    void setTimeSerie(bool state) {
         clear();
         m_autoScaleTime = state;
    }
    void unsetMinMax() {
        if(m_autoScale)
            return;
        m_autoScale = true;
        //Recalculate the automatic scale
        if(m_data.isEmpty())
            minMaxSet = false;
        else {
            minMaxSet = true;
            previousTime = xmax = xmin = m_data.at(0).x();
            ymax = ymin = m_data.at(0).y();
            for(int i = 1; i < m_data.size(); i++) {
                xmin = qMin(qreal(xmin), m_data.at(i).x());
                xmax = qMax(qreal(xmax), m_data.at(i).x());
                ymin = qMin(qreal(ymin), m_data.at(i).y());
                ymax = qMax(qreal(ymax), m_data.at(i).y());
            }
        }
    }
    void setMinMax(double xmin, double xmax, double ymin, double ymax )
    {
        this->xmin = xmin;
        this->xmax = xmax;
        this->ymin = ymin;
        this->ymax = ymax;
        m_autoScale = false;
        minMaxSet = true;
        itemChanged();
    }
    void setStartIndex(int time) {  /** Set to -1 to just use latest */
        m_startIndex = time;
        itemChanged();
    }
    int dataSize() const { return m_data.size(); }

    double xMin() const { return xmin; }
    double xMax() const { return xmax; }
    double yMin() const { return ymin; }
    double yMax() const { return ymax; }

    virtual QRectF boundingRect() const {
        if(!minMaxSet)
            return QRectF(1,1,-2,-2);
        return QRectF(xmin,ymin,xmax-xmin,ymax-ymin);
    }

    /* From QwtPlotItem.  Draw the complete series */
    virtual void draw (QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const
    {
        Q_UNUSED(canvasRect);
        QPointF lastPoint;
        if(m_data.isEmpty())
            return;
        QPointF smoothTotal(0,0);
        int smoothCount = 0;
        int start;
        int count;
        if(m_startIndex >= 0) {
            int end = qMin(m_startIndex, m_data.size()-1);
            start = qBound(0, end - m_maxShowPoints, m_data.size()-1);
            count = end - start;
        } else {
            start = qMax(0,m_data.size() - m_maxShowPoints);
            count = qMin(m_data.size()-start, m_maxShowPoints);
        }
        for(int i = qMax(0,start - m_smoothPoints); i < start; ++i) {
            smoothTotal += m_data.at(i);
            ++smoothCount;
        }
        for(int i = 0; i < count; ++i) {
            QPointF point = m_data.at(i+start);
            if(m_smoothPoints > 1) {
                smoothTotal += point;
                if(smoothCount >= m_smoothPoints) {
                    Q_ASSERT(i + start - m_smoothPoints >= 0);
                    smoothTotal -= m_data.at(i + start - m_smoothPoints);
                } else
                    ++smoothCount;
                point = smoothTotal/smoothCount;
            }
            QPointF paintCoord = QPointF(xMap.transform(point.x()), yMap.transform(point.y()));
            m_color.setAlpha((m_maxShowPoints - count + i)*255/m_maxShowPoints);
            p->setPen(m_color);
            if(i != 0)
                p->drawLine(lastPoint, paintCoord);
            if(i == count-1) {
                //Draw marker for first point
                const int marker_radius = 2;
                QRectF marker = QRectF(paintCoord.x()-marker_radius, paintCoord.y()-marker_radius, marker_radius*2+1,marker_radius*2+1);
                p->fillRect(marker,QBrush(m_color));
            }
            lastPoint = paintCoord;
        }
    }

private:
    QList< QPointF > m_data;
    int m_maxStorePoints;
    int m_maxShowPoints;
    int m_smoothPoints; /** Number of points to average across */
    mutable QColor m_color;

    double previousTime;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    bool minMaxSet;
    bool m_autoScale;
    bool m_autoScaleTime;
    int m_startIndex;
};

QGCXYPlot::QGCXYPlot(QWidget *parent) :
    QGCToolWidgetItem("XY Plot", parent),
    ui(new Ui::QGCXYPlot),
    plot(0),
    xycurve(0),
    x(0),
    x_timestamp_us(0),
    x_valid(false),
    y(0),
    y_timestamp_us(0),
    y_valid(false),
    max_timestamp_diff_us(10000) /* Default to 10ms tolerance between x and y values */


{
    uas = 0;
    ui->setupUi(this);
    plot = new QwtPlot();

    ui->xyPlotLayout->addWidget(plot);

    connect(ui->editFinishButton, SIGNAL(clicked()), this, SLOT(endEditMode()));

    connect(MainWindow::instance(), SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),
            this, SLOT(appendData(int,QString,QString,QVariant,quint64)));

    connect(ui->editXParam, SIGNAL(editTextChanged(QString)), this, SLOT(clearPlot()));
    connect(ui->editYParam, SIGNAL(editTextChanged(QString)), this, SLOT(clearPlot()));

    plot->plotLayout()->setAlignCanvasToScales(true);

    QwtLinearScaleEngine* yScaleEngine = new QwtLinearScaleEngine();
    plot->setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);

    plot->setAxisAutoScale(QwtPlot::xBottom);
    plot->setAxisAutoScale(QwtPlot::yLeft);
    plot->setAutoReplot();
    xycurve = new XYPlotCurve();
    xycurve->attach(plot);
    styleChanged(qgcApp()->styleIsDark());
    connect(qgcApp(), &QGCApplication::styleChanged, this, &QGCXYPlot::styleChanged);
    connect(ui->minX, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->maxX, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->minY, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->maxY, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->automaticAxisRange, SIGNAL(toggled(bool)),this, SLOT(updateMinMaxSettings()));
    connect(ui->timeAxisRange, SIGNAL(toggled(bool)),this, SLOT(setTimeAxis()));
    connect(ui->maxDataShowSpinBox, SIGNAL(valueChanged(int)),this, SLOT(updateMinMaxSettings()));
    connect(ui->maxDataStoreSpinBox, SIGNAL(valueChanged(int)),this, SLOT(updateMinMaxSettings()));
    connect(ui->smoothSpinBox, SIGNAL(valueChanged(int)),this, SLOT(updateMinMaxSettings()));
    setEditMode(false);
}

QGCXYPlot::~QGCXYPlot()
{
    delete ui;
}

void QGCXYPlot::clearPlot()
{
    xycurve->clear();
    plot->detachItems();
    ui->timeScrollBar->setMaximum(xycurve->dataSize());
    ui->timeScrollBar->setValue(ui->timeScrollBar->maximum());
}

void QGCXYPlot::setEditMode(bool editMode)
{
    ui->lblXParam->setVisible(editMode);
    ui->lblYParam->setVisible(editMode);
    ui->editXParam->setVisible(editMode);
    ui->editYParam->setVisible(editMode);
    ui->editFinishButton->setVisible(editMode);
    ui->editLine1->setVisible(editMode);
    ui->editLine2->setVisible(editMode);
    ui->lblMaxDataStore->setVisible(editMode);
    ui->lblMaxDataShow->setVisible(editMode);
    ui->lblMaxX->setVisible(editMode);
    ui->lblMaxY->setVisible(editMode);
    ui->lblMinX->setVisible(editMode);
    ui->lblMinY->setVisible(editMode);
    ui->maxX->setVisible(editMode);
    ui->maxY->setVisible(editMode);
    ui->minX->setVisible(editMode);
    ui->minY->setVisible(editMode);
    ui->maxDataShowSpinBox->setVisible(editMode);
    ui->maxDataStoreSpinBox->setVisible(editMode);
    ui->automaticAxisRange->setVisible(editMode);
    ui->timeAxisRange->setVisible(editMode);
    ui->lblSmooth->setVisible(editMode);
    ui->smoothSpinBox->setVisible(editMode);

    if(!editMode) {
        plot->setAxisTitle(QwtPlot::xBottom, ui->editXParam->currentText());
        plot->setAxisTitle(QwtPlot::yLeft, ui->editYParam->currentText());
    }

    QGCToolWidgetItem::setEditMode(editMode);
    updateMinMaxSettings(); //Do this after calling the parent
}

void QGCXYPlot::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "XYPLOT");
    settings.setValue("QGC_XYPLOT_X", ui->editXParam->currentText());
    settings.setValue("QGC_XYPLOT_Y", ui->editYParam->currentText());
    settings.setValue("QGC_XYPLOT_MINX", ui->minX->value());
    settings.setValue("QGC_XYPLOT_MAXX", ui->maxX->value());
    settings.setValue("QGC_XYPLOT_MINY", ui->minY->value());
    settings.setValue("QGC_XYPLOT_MAXY", ui->maxY->value());
    settings.setValue("QGC_XYPLOT_MAXDATA_STORE", ui->maxDataStoreSpinBox->value());
    settings.setValue("QGC_XYPLOT_MAXDATA_SHOW", ui->maxDataShowSpinBox->value());
    settings.setValue("QGC_XYPLOT_AUTO", ui->automaticAxisRange->isChecked());
    settings.setValue("QGC_XYPLOT_SMOOTH", ui->smoothSpinBox->value());
    settings.setValue("QGC_XYPLOT_TIME", ui->timeAxisRange->isChecked());
    settings.sync();
}
void QGCXYPlot::readSettings(const QString& pre,const QVariantMap& settings)
{
    ui->editXParam->setEditText(settings.value(pre + "QGC_XYPLOT_X", "").toString());
    ui->editYParam->setEditText(settings.value(pre + "QGC_XYPLOT_Y", "").toString());
    ui->automaticAxisRange->setChecked(settings.value(pre + "QGC_XYPLOT_AUTO", true).toBool());
    ui->minX->setValue(settings.value(pre + "QGC_XYPLOT_MINX", 0).toDouble());
    ui->maxX->setValue(settings.value(pre + "QGC_XYPLOT_MAXX", 0).toDouble());
    ui->minY->setValue(settings.value(pre + "QGC_XYPLOT_MINY", 0).toDouble());
    ui->maxY->setValue(settings.value(pre + "QGC_XYPLOT_MAXY", 0).toDouble());
    ui->maxDataStoreSpinBox->setValue(settings.value(pre + "QGC_XYPLOT_MAXDATA_STORE", 10000).toInt());
    ui->maxDataShowSpinBox->setValue(settings.value(pre + "QGC_XYPLOT_MAXDATA_SHOW", 15).toInt());
    ui->smoothSpinBox->setValue(settings.value(pre + "QGC_XYPLOT_SMOOTH", 1).toInt());
    ui->timeAxisRange->setChecked(settings.value(pre + "QGC_XYPLOT_TIME", true).toBool());
    plot->setAxisTitle(QwtPlot::xBottom, ui->editXParam->currentText());
    plot->setAxisTitle(QwtPlot::yLeft, ui->editYParam->currentText());
    updateMinMaxSettings();
}

void QGCXYPlot::readSettings(const QSettings& settings)
{
    ui->editXParam->setEditText(settings.value("QGC_XYPLOT_X", "").toString());
    ui->editYParam->setEditText(settings.value("QGC_XYPLOT_Y", "").toString());
    ui->automaticAxisRange->setChecked(settings.value("QGC_XYPLOT_AUTO", true).toBool());
    ui->minX->setValue(settings.value("QGC_XYPLOT_MINX", 0).toDouble());
    ui->maxX->setValue(settings.value("QGC_XYPLOT_MAXX", 0).toDouble());
    ui->minY->setValue(settings.value("QGC_XYPLOT_MINY", 0).toDouble());
    ui->maxY->setValue(settings.value("QGC_XYPLOT_MAXY", 0).toDouble());
    ui->maxDataStoreSpinBox->setValue(settings.value("QGC_XYPLOT_MAXDATA_STORE", 10000).toInt());
    ui->maxDataShowSpinBox->setValue(settings.value("QGC_XYPLOT_MAXDATA_SHOW", 15).toInt());
    ui->smoothSpinBox->setValue(settings.value("QGC_XYPLOT_SMOOTH", 1).toInt());
    ui->timeAxisRange->setChecked(settings.value("QGC_XYPLOT_TIME", true).toBool());
    plot->setAxisTitle(QwtPlot::xBottom, ui->editXParam->currentText());
    plot->setAxisTitle(QwtPlot::yLeft, ui->editYParam->currentText());
    updateMinMaxSettings();
}

void QGCXYPlot::appendData(int uasId, const QString& curve, const QString& unit, const QVariant& variant, quint64 usec)
{
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    if(isEditMode()) {
        //When in edit mode, add all the items to the combo box
        if(ui->editXParam->findText(curve) == -1) {
            ui->editXParam->blockSignals(true);
            ui->editYParam->blockSignals(true);
            QString oldX = ui->editXParam->currentText();
            QString oldY = ui->editYParam->currentText();
            ui->editXParam->addItem(curve); //Annoyingly this can wipe out the current text
            ui->editYParam->addItem(curve);
            ui->editXParam->setEditText(oldX);
            ui->editYParam->setEditText(oldY);
            ui->editXParam->blockSignals(false);
            ui->editYParam->blockSignals(false);
        }
    }

    if(ui->stopStartButton->isChecked())
        return;

    bool ok;
    if(curve == ui->editXParam->currentText()) {
        x = variant.toDouble(&ok);
        if(!ok)
            return;
        x_timestamp_us = usec;
        x_valid = true;
    } else if(curve == ui->editYParam->currentText()) {
        y = variant.toDouble(&ok);
        if(!ok)
            return;
        y_timestamp_us = usec;
        y_valid = true;
    } else
        return;

    if(x_valid && y_valid) {
        quint64 difference;
        if (y_timestamp_us < x_timestamp_us) {
            difference = x_timestamp_us - y_timestamp_us;
        } else {
            difference = y_timestamp_us - x_timestamp_us;
        }
        if (difference <= max_timestamp_diff_us) {
            int removed = xycurve->appendData( QPointF(x,y) );
            x_valid = false;
            y_valid = false;
            bool atMaximum = (ui->timeScrollBar->value() == ui->timeScrollBar->maximum());
            if(ui->timeScrollBar->maximum() != xycurve->dataSize()) {
                ui->timeScrollBar->setMaximum(xycurve->dataSize());
                if(atMaximum)
                    ui->timeScrollBar->setValue(ui->timeScrollBar->maximum());
            } else if(!atMaximum) { //Move the scrollbar to keep current value selected
                int value = qMax(ui->timeScrollBar->minimum(), ui->timeScrollBar->value() - removed);
                ui->timeScrollBar->setValue(value);
                xycurve->setStartIndex(value);
            }
        }
    }
}

void QGCXYPlot::styleChanged(bool styleIsDark)
{
    xycurve->setColor(styleIsDark ? Qt::white : Qt::black);
}

void QGCXYPlot::updateMinMaxSettings()
{
    bool automatic = ui->automaticAxisRange->isChecked();
    ui->minX->setEnabled(!automatic);
    ui->maxX->setEnabled(!automatic);
    ui->minY->setEnabled(!automatic);
    ui->maxY->setEnabled(!automatic);
    if(automatic) {
        xycurve->unsetMinMax();
    } else {
        xycurve->setMinMax(ui->minX->value(), ui->maxX->value(), ui->minY->value(), ui->maxY->value());
    }
    xycurve->setMaxDataStorePoints(ui->maxDataStoreSpinBox->value());
    xycurve->setMaxDataShowPoints(ui->maxDataShowSpinBox->value());
    xycurve->setSmoothPoints(ui->smoothSpinBox->value());
}

void QGCXYPlot::setTimeAxis()
{
    xycurve->setTimeSerie(ui->timeAxisRange->isChecked());
}

void QGCXYPlot::on_maxDataShowSpinBox_valueChanged(int value)
{
    ui->maxDataStoreSpinBox->setMinimum(value);
    if(ui->maxDataStoreSpinBox->value() < value)
        ui->maxDataStoreSpinBox->setValue(value);
}

void QGCXYPlot::on_stopStartButton_toggled(bool checked)
{
    if(!checked)
        clearPlot();
}

void QGCXYPlot::on_timeScrollBar_valueChanged(int value)
{
    if(value == ui->timeScrollBar->maximum())
        xycurve->setStartIndex(-1);
    else
        xycurve->setStartIndex(value);
}

#include <QDockWidget>

#include "QGCXYPlot.h"
#include "ui_QGCXYPlot.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"
#include "IncrementalPlot.h"
#include <float.h>
#include <qwt_plot.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_engine.h>

class XYPlotCurve : public QwtPlotItem
{
public:
    XYPlotCurve() {
        m_maxPoints = 15;
        setItemAttribute(QwtPlotItem::AutoScale);
        minMaxSet = false;
        m_color = Qt::white;
    }

    void setMaxDataPoints(int max) { m_maxPoints = max; }
    int maxPoints() const { return m_maxPoints; }

    void appendData(const QPointF &data) {
        if(!minMaxSet) {
            xmin = xmax = data.x();
            ymin = ymax = data.y();
            minMaxSet = true;
        } else if(m_autoScale) {
            xmin = qMin(xmin, data.x());
            xmax = qMax(xmax, data.x());
            ymin = qMin(ymin, data.y());
            ymax = qMax(ymax, data.y());
        }


        m_data.append(data);
        while(m_data.size() > m_maxPoints)
            m_data.removeFirst();
        itemChanged();
    }
    void clear() {
        minMaxSet = false;
        m_data.clear();
        itemChanged();
    }
    void setColor(const QColor &color) {
        m_color = color;
    }
    void unsetMinMax() {
        if(m_autoScale)
            return;
        m_autoScale = true;
        clear();
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

    double xMin() const { return xmin; }
    double xMax() const { return xmax; }
    double yMin() const { return ymin; }
    double yMax() const { return ymax; }

    virtual QwtDoubleRect boundingRect() const {
        if(!minMaxSet)
            return QwtDoubleRect(1,1,-2,-2);
        return QwtDoubleRect(xmin,ymin,xmax-xmin,ymax-ymin);
    }

protected:
    /* From QwtPlotItem.  Draw the complete series */
    virtual void draw (QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &canvasRect) const
    {
        Q_UNUSED(canvasRect);
        QPointF lastPoint;
        int i = 0;
        if(m_data.isEmpty())
            return;
        int dataSize = m_data.size();

        foreach(const QPointF &point, m_data) {
            QPointF paintCoord = QPointF(xMap.xTransform(point.x()), yMap.xTransform(point.y()));
            m_color.setAlpha((i+m_maxPoints - dataSize)*255/m_maxPoints);
            p->setPen(m_color);
            if(i++)
                p->drawLine(lastPoint, paintCoord);
            if(i == dataSize) {
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
    int m_maxPoints;
    mutable QColor m_color;

    double xmin;
    double xmax;
    double ymin;
    double ymax;
    bool minMaxSet;
    bool m_autoScale;
};

QGCXYPlot::QGCXYPlot(QWidget *parent) :
    QGCToolWidgetItem("XY Plot", parent),
    ui(new Ui::QGCXYPlot),
    plot(0),
    xycurve(0),
    maxElementsToDraw(5),
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

    QHBoxLayout* layout = new QHBoxLayout(ui->xyPlotFrame);
    layout->addWidget(plot);

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
    styleChanged(MainWindow::instance()->getStyle());
    connect(MainWindow::instance(), SIGNAL(styleChanged(MainWindow::QGC_MAINWINDOW_STYLE)),
            this, SLOT(styleChanged(MainWindow::QGC_MAINWINDOW_STYLE)));
    connect(ui->minX, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->maxX, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->minY, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->maxY, SIGNAL(valueChanged(double)),this, SLOT(updateMinMaxSettings()));
    connect(ui->automaticAxisRange, SIGNAL(toggled(bool)),this, SLOT(updateMinMaxSettings()));
    connect(ui->maxDataSpinBox, SIGNAL(valueChanged(int)),this, SLOT(updateMinMaxSettings()));
    setEditMode(false);
}

QGCXYPlot::~QGCXYPlot()
{
    delete ui;
}

void QGCXYPlot::clearPlot()
{
    xycurve->clear();
    plot->clear();
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
    ui->lblMaxData->setVisible(editMode);
    ui->lblMaxX->setVisible(editMode);
    ui->lblMaxY->setVisible(editMode);
    ui->lblMinX->setVisible(editMode);
    ui->lblMinY->setVisible(editMode);
    ui->maxX->setVisible(editMode);
    ui->maxY->setVisible(editMode);
    ui->minX->setVisible(editMode);
    ui->minY->setVisible(editMode);
    ui->maxDataSpinBox->setVisible(editMode);
    ui->automaticAxisRange->setVisible(editMode);

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
    settings.setValue("QGC_XYPLOT_MAXDATA", ui->maxDataSpinBox->value());
    settings.setValue("QGC_XYPLOT_AUTO", ui->automaticAxisRange->isChecked());

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
    ui->maxDataSpinBox->setValue(settings.value(pre + "QGC_XYPLOT_MAXDATA", 15).toInt());
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
    ui->maxDataSpinBox->setValue(settings.value("QGC_XYPLOT_MAXDATA", 15).toInt());
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
            QString oldX = ui->editXParam->currentText();
            QString oldY = ui->editYParam->currentText();
            ui->editXParam->addItem(curve); //Annoyingly this can wipe out the current text
            ui->editYParam->addItem(curve);
            ui->editXParam->setEditText(oldX);
            ui->editYParam->setEditText(oldY);
        }
    }

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

    if(x_valid && y_valid && (int)qAbs(y_timestamp_us - x_timestamp_us) <= max_timestamp_diff_us) {
        xycurve->appendData( QPointF(x,y) );
        plot->update();
        x_valid = false;
        y_valid = false;
    }
}

void QGCXYPlot::styleChanged(MainWindow::QGC_MAINWINDOW_STYLE style)
{
    if (style == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT)
        xycurve->setColor(Qt::black);
    else
        xycurve->setColor(Qt::white);
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
    xycurve->setMaxDataPoints(ui->maxDataSpinBox->value());
}

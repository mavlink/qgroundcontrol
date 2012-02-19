/*=====================================================================
======================================================================*/

/**
 * @file
 *   @brief Implementation of Head Down Display (HDD)
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QGLWidget>
#include <QStringList>
#include <QGraphicsTextItem>
#include <QDockWidget>
#include <QInputDialog>
#include <QMouseEvent>
#include <QMenu>
#include <QSettings>
#include <qmath.h>
#include "UASManager.h"
#include "HDDisplay.h"
#include "ui_HDDisplay.h"
#include "MG.h"
#include "QGC.h"
#include <QDebug>

HDDisplay::HDDisplay(QStringList* plotList, QString title, QWidget *parent) :
    QGraphicsView(parent),
    uas(NULL),
    xCenterOffset(0.0f),
    yCenterOffset(0.0f),
    vwidth(80.0f),
    vheight(80.0f),
    backgroundColor(QColor(0, 0, 0)),
    defaultColor(QColor(70, 200, 70)),
    setPointColor(QColor(200, 20, 200)),
    warningColor(Qt::yellow),
    criticalColor(Qt::red),
    infoColor(QColor(20, 200, 20)),
    fuelColor(criticalColor),
    warningBlinkRate(5),
    refreshTimer(new QTimer(this)),
    hardwareAcceleration(true),
    strongStrokeWidth(1.5f),
    normalStrokeWidth(1.0f),
    fineStrokeWidth(0.5f),
    acceptList(new QStringList()),
    acceptUnitList(new QStringList()),
    lastPaintTime(0),
    columns(3),
    valuesChanged(true),
    m_ui(NULL)
{
    setWindowTitle(title);
    //m_ui->setupUi(this);

    setAutoFillBackground(true);

    // Add all items in accept list to gauge
    if (plotList) {
        for(int i = 0; i < plotList->length(); ++i) {
            addGauge(plotList->at(i));
        }
    }

    restoreState();
    // Set preferred size
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    createActions();

    //    setBackgroundBrush(QBrush(backgroundColor));
    //    setDragMode(QGraphicsView::ScrollHandDrag);
    //    setCacheMode(QGraphicsView::CacheBackground);
    //    // FIXME Handle full update with care - ressource intensive
    //    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    //
    //    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    //
    //    //Set-up the scene
    //    QGraphicsScene* Scene = new QGraphicsScene(this);
    //    setScene(Scene);
    //
    //    //Populate the scene
    //    for(int x = 0; x < 1000; x = x + 25) {
    //        for(int y = 0; y < 1000; y = y + 25) {
    //
    //            if(x % 100 == 0 && y % 100 == 0) {
    //                Scene->addRect(x, y, 2, 2);
    //
    //                QString pointString;
    //                QTextStream stream(&pointString);
    //                stream << "(" << x << "," << y << ")";
    //                QGraphicsTextItem* item = Scene->addText(pointString);
    //                item->setPos(x, y);
    //            } else {
    //                Scene->addRect(x, y, 1, 1);
    //            }
    //        }
    //    }
    //
    //    //Set-up the view
    //    setSceneRect(0, 0, 1000, 1000);
    //    setCenter(QPointF(500.0, 500.0)); //A modified version of centerOn(), handles special cases
    //    setCursor(Qt::OpenHandCursor);


    // Set minimum size
    this->setMinimumHeight(125);
    this->setMinimumWidth(100);

    scalingFactor = this->width()/vwidth;

    // Refresh timer
    refreshTimer->setInterval(180); //
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(triggerUpdate()));
    //connect(refreshTimer, SIGNAL(timeout()), this, SLOT(paintGL()));

    fontDatabase = QFontDatabase();
    const QString fontFileName = ":/general/vera.ttf"; ///< Font file is part of the QRC file and compiled into the app
    const QString fontFamilyName = "Bitstream Vera Sans";
    if(!QFile::exists(fontFileName)) qDebug() << "ERROR! font file: " << fontFileName << " DOES NOT EXIST!";

    fontDatabase.addApplicationFont(fontFileName);
    font = fontDatabase.font(fontFamilyName, "Roman", qMax(5, (int)(10*scalingFactor*1.2f+0.5f)));
    if (font.family() != fontFamilyName) qDebug() << "ERROR! Font not loaded: " << fontFamilyName;

    // Connect with UAS
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    //start();
}

HDDisplay::~HDDisplay()
{
    saveState();
	if(this->refreshTimer)
	{
		delete this->refreshTimer;
	}
	if(this->acceptList)
	{
		delete this->acceptList;
	}
	if(this->acceptUnitList)
	{
		delete this->acceptUnitList;
	}
	if(this->m_ui)
	{
		delete m_ui;
	}
}

QSize HDDisplay::sizeHint() const
{
    return QSize(400, 400.0f*(vwidth/vheight)*1.2f);
}

void HDDisplay::enableGLRendering(bool enable)
{
    Q_UNUSED(enable);
}

void HDDisplay::triggerUpdate()
{
    // Only repaint the regions necessary
    update(this->geometry());
}

//void HDDisplay::updateValue(UASInterface* uas, const QString& name, const QString& unit, double value, quint64 msec)
//{
//    // UAS is not needed
//    Q_UNUSED(uas);

//    if (!isnan(value) && !isinf(value))
//    {
//        // Update mean
//        const float oldMean = valuesMean.value(name, 0.0f);
//        const int meanCount = valuesCount.value(name, 0);
//        double mean = (oldMean * meanCount +  value) / (meanCount + 1);
//        if (isnan(mean) || isinf(mean)) mean = 0.0;
//        valuesMean.insert(name, mean);
//        valuesCount.insert(name, meanCount + 1);
//        // Two-value sliding average
//        double dot = (valuesDot.value(name) + (value - values.value(name, 0.0f)) / ((msec - lastUpdate.value(name, 0))/1000.0f))/2.0f;
//        if (isnan(dot) || isinf(dot))
//        {
//            dot = 0.0;
//        }
//        valuesDot.insert(name, dot);
//        values.insert(name, value);
//        lastUpdate.insert(name, msec);
//        //}

//        //qDebug() << __FILE__ << __LINE__ << "VALUE:" << value << "MEAN:" << mean << "DOT:" << dot << "COUNT:" << meanCount;
//    }
//}

void HDDisplay::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);
    quint64 interval = 0;
    //qDebug() << "INTERVAL:" << MG::TIME::getGroundTimeNow() - interval << __FILE__ << __LINE__;
    interval = QGC::groundTimeMilliseconds();
    renderOverlay();
}

void HDDisplay::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(addGaugeAction);
    menu.addActions(getItemRemoveActions());
    menu.addSeparator();
    menu.addAction(setColumnsAction);
    // Title change would ruin settings
    // this can only be allowed once
    // HDDisplays are instantiated
    // by a factory method based on
    // QSettings
    //menu.addAction(setTitleAction);
    menu.exec(event->globalPos());
}

void HDDisplay::saveState()
{
    QSettings settings;

    QString instruments;
    // Restore instrument settings
    for (int i = 0; i < acceptList->count(); i++) {
        QString key = acceptList->at(i);
        instruments += "|" + QString::number(minValues.value(key, -1.0))+","+key+","+acceptUnitList->at(i)+","+QString::number(maxValues.value(key, +1.0))+","+customNames.value(key, "")+","+((symmetric.value(key, false)) ? "s" : "");
    }

    // qDebug() << "Saving" << instruments;

    settings.setValue(windowTitle()+"_gauges", instruments);
    settings.sync();
}

void HDDisplay::restoreState()
{
    QSettings settings;
    settings.sync();

    acceptList->clear();

    QStringList instruments = settings.value(windowTitle()+"_gauges").toString().split('|');
    for (int i = 0; i < instruments.count(); i++) {
        addGauge(instruments.at(i));
    }
}

QList<QAction*> HDDisplay::getItemRemoveActions()
{
    QList<QAction*> actions;
    for(int i = 0; i < acceptList->length(); ++i) {
        QString gauge = acceptList->at(i);
        QAction* remove = new QAction(tr("Remove %1 gauge").arg(gauge), this);
        remove->setStatusTip(tr("Removes the %1 gauge from the view.").arg(gauge));
        remove->setData(gauge);
        connect(remove, SIGNAL(triggered()), this, SLOT(removeItemByAction()));
        actions.append(remove);
    }
    return actions;
}

void HDDisplay::removeItemByAction()
{
    QAction* trigger = qobject_cast<QAction*>(QObject::sender());
    if (trigger) {
        QString item = trigger->data().toString();
        int index = acceptList->indexOf(item);
        acceptList->removeAt(index);
        minValues.remove(item);
        maxValues.remove(item);
        symmetric.remove(item);
        adjustGaugeAspectRatio();
    }
}

void HDDisplay::addGauge()
{
    QStringList items;
    for (int i = 0; i < values.count(); ++i) {
        QString key = values.keys().at(i);
        QString label = key;
        QStringList keySplit = key.split(".");
        if (keySplit.size() > 1)
        {
            keySplit.removeFirst();
            label = keySplit.join(".");
        }
        QString unit = units.value(key);
        if (unit.contains("deg") || unit.contains("rad")) {
            items.append(QString("%1,%2,%3,%4,%5,s").arg("-180").arg(key).arg(unit).arg("+180").arg(label));
        } else {
            items.append(QString("%1,%2,%3,%4,%5").arg("0").arg(key).arg(unit).arg("+100").arg(label));
        }
    }
    bool ok;
    QString item = QInputDialog::getItem(this, tr("Add Gauge Instrument"),
                                         tr("Format: min, data name, unit, max, label [,s]"), items, 0, true, &ok);
    if (ok && !item.isEmpty()) {
        addGauge(item);
    }
}

void HDDisplay::addGauge(const QString& gauge)
{
    if (gauge.length() > 0) {
        QStringList parts = gauge.split(',');
        if (parts.count() > 2) {
            double val;
            bool ok;
            bool success = true;

            QString key = parts.at(1);
            QString unit = parts.at(2);

            if (!acceptList->contains(key)) {
                // Convert min to double number
                val = parts.first().toDouble(&ok);
                success &= ok;
                if (ok) minValues.insert(key, val);
                // Convert max to double number
                val = parts.at(3).toDouble(&ok);
                success &= ok;
                if (ok) maxValues.insert(key, val);
                // Convert name
                if (parts.length() >= 5)
                {
                    if (parts.at(4).length() > 0)
                    {
                        customNames.insert(key, parts.at(4));
                    }
                }
                // Convert symmetric flag
                if (parts.length() >= 6)
                {
                    if (parts.at(5).contains("s"))
                    {
                        symmetric.insert(key, true);
                    }
                }
                if (success) {
                    // Add value to acceptlist
                    acceptList->append(key);
                    acceptUnitList->append(unit);
                }
            }
        } else if (parts.count() > 1) {
            if (!acceptList->contains(gauge)) {
                acceptList->append(parts.at(0));
                acceptUnitList->append(parts.at(1));
            }
        }
    }
    adjustGaugeAspectRatio();
}

void HDDisplay::createActions()
{
    addGaugeAction = new QAction(tr("New &Gauge"), this);
    addGaugeAction->setStatusTip(tr("Add a new gauge to the view by adding its name from the linechart"));
    connect(addGaugeAction, SIGNAL(triggered()), this, SLOT(addGauge()));

    setTitleAction = new QAction(tr("Set Widget Title"), this);
    setTitleAction->setStatusTip(tr("Set the title caption of this tool widget"));
    connect(setTitleAction, SIGNAL(triggered()), this, SLOT(setTitle()));

    setColumnsAction = new QAction(tr("Set Number of Instrument Columns"), this);
    setColumnsAction->setStatusTip(tr("Set number of columns to draw"));
    connect(setColumnsAction, SIGNAL(triggered()), this, SLOT(setColumns()));
}


void HDDisplay::setColumns()
{
    bool ok;
    int i = QInputDialog::getInt(this, tr("Number of Instrument Columns"),
                                 tr("Columns:"), columns, 1, 15, 1, &ok);
    if (ok) {
        columns = i;
    }
}

void HDDisplay::setColumns(int cols)
{
    columns = cols;
    adjustGaugeAspectRatio();
}

void HDDisplay::adjustGaugeAspectRatio()
{
    // Adjust vheight dynamically according to the number of rows
    float vColWidth = vwidth / columns;
    int vRows = ceil(acceptList->length()/(float)columns);
    // Assuming square instruments, vheight is column width*row count
    vheight = vColWidth * vRows;
}

void HDDisplay::setTitle()
{
    QDockWidget* parent = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (parent) {
        bool ok;
        QString text = QInputDialog::getText(this, tr("New title"),
                                             tr("Widget title:"), QLineEdit::Normal,
                                             parent->windowTitle(), &ok);
        if (ok && !text.isEmpty())
            parent->setWindowTitle(text);
        this->setWindowTitle(text);
    }
}

void HDDisplay::renderOverlay()
{
    if (!valuesChanged || !isVisible()) return;

#if (QGC_EVENTLOOP_DEBUG)
    qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif
    quint64 refreshInterval = 100;
    quint64 currTime = MG::TIME::getGroundTimeNow();
    if (currTime - lastPaintTime < refreshInterval) {
        // FIXME Need to find the source of the spurious paint events
        //return;
    }
    lastPaintTime = currTime;
    // Draw instruments
    // TESTING THIS SHOULD BE MOVED INTO A QGRAPHICSVIEW
    // Update scaling factor
    // adjust scaling to fit both horizontally and vertically
    scalingFactor = this->width()/vwidth;

    double scalingFactorH = this->height()/vheight;
    if (scalingFactorH < scalingFactor) scalingFactor = scalingFactorH;

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    //painter.fillRect(QRect(0, 0, width(), height()), backgroundColor);
    const float spacing = 0.4f; // 40% of width
    const float gaugeWidth = vwidth / (((float)columns) + (((float)columns+1) * spacing + spacing * 0.5f));
    const QColor gaugeColor = QColor(200, 200, 200);
    //drawSystemIndicator(10.0f-gaugeWidth/2.0f, 20.0f, 10.0f, 40.0f, 15.0f, &painter);
    //drawGauge(15.0f, 15.0f, gaugeWidth/2.0f, 0, 1.0f, "thrust", values.value("thrust", 0.0f), gaugeColor, &painter, qMakePair(0.45f, 0.8f), qMakePair(0.8f, 1.0f), true);
    //drawGauge(15.0f+gaugeWidth*1.7f, 15.0f, gaugeWidth/2.0f, 0, 10.0f, "altitude", values.value("altitude", 0.0f), gaugeColor, &painter, qMakePair(1.0f, 2.5f), qMakePair(0.0f, 0.5f), true);

    // Left spacing from border / other gauges, measured from left edge to center
    float leftSpacing = gaugeWidth * spacing;
    float xCoord = leftSpacing + gaugeWidth/2.0f;

    float topSpacing = leftSpacing;
    float yCoord = topSpacing + gaugeWidth/2.0f;

    for (int i = 0; i < acceptList->size(); ++i)
    {
        QString value = acceptList->at(i);
        QString label = customNames.value(value);
        drawGauge(xCoord, yCoord, gaugeWidth/2.0f, minValues.value(value, -1.0f), maxValues.value(value, 1.0f), label, values.value(value, minValues.value(value, 0.0f)), gaugeColor, &painter, symmetric.value(value, false), goodRanges.value(value, qMakePair(0.0f, 0.5f)), critRanges.value(value, qMakePair(0.7f, 1.0f)), true);
        xCoord += gaugeWidth + leftSpacing;
        // Move one row down if necessary
        if (xCoord + gaugeWidth*0.9f > vwidth)
        {
            yCoord += topSpacing + gaugeWidth;
            xCoord = leftSpacing + gaugeWidth/2.0f;
        }
    }
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HDDisplay::setActiveUAS(UASInterface* uas)
{
    // Disconnect any previously connected active UAS
    if (this->uas != NULL) {
        removeSource(this->uas);
    }

    // Now connect the new UAS
	addSource(uas);
    this->uas = uas;
}

/**
 * Rotate a polygon around a point
 *
 * @param p polygon to rotate
 * @param origin the rotation center
 * @param angle rotation angle, in radians
 * @return p Polygon p rotated by angle around the origin point
 */
void HDDisplay::rotatePolygonClockWiseRad(QPolygonF& p, float angle, QPointF origin)
{
    // Standard 2x2 rotation matrix, counter-clockwise
    //
    //   |  cos(phi)   sin(phi) |
    //   | -sin(phi)   cos(phi) |
    //
    for (int i = 0; i < p.size(); i++) {
        QPointF curr = p.at(i);

        const float x = curr.x();
        const float y = curr.y();

        curr.setX(((cos(angle) * (x-origin.x())) + (-sin(angle) * (y-origin.y()))) + origin.x());
        curr.setY(((sin(angle) * (x-origin.x())) + (cos(angle) * (y-origin.y()))) + origin.y());
        p.replace(i, curr);
    }
}

void HDDisplay::drawPolygon(QPolygonF refPolygon, QPainter* painter)
{
    // Scale coordinates
    QPolygonF draw(refPolygon.size());
    for (int i = 0; i < refPolygon.size(); i++) {
        QPointF curr;
        curr.setX(refToScreenX(refPolygon.at(i).x()));
        curr.setY(refToScreenY(refPolygon.at(i).y()));
        draw.replace(i, curr);
    }
    painter->drawPolygon(draw);
}

void HDDisplay::drawChangeRateStrip(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter)
{
    QBrush brush(defaultColor, Qt::NoBrush);
    painter->setBrush(brush);
    QPen rectPen(Qt::SolidLine);
    rectPen.setWidth(0);
    rectPen.setColor(defaultColor);
    painter->setPen(rectPen);

    float scaledValue = value;

    // Saturate value
    if (value > maxRate) scaledValue = maxRate;
    if (value < minRate) scaledValue = minRate;

    //           x (Origin: xRef, yRef)
    //           -
    //           |
    //           |
    //           |
    //           =
    //           |
    //   -0.005 >|
    //           |
    //           -

    const float width = height / 8.0f;
    const float lineWidth = 0.5f;

    // Indicator lines
    // Top horizontal line
    drawLine(xRef, yRef, xRef+width, yRef, lineWidth, defaultColor, painter);
    // Vertical main line
    drawLine(xRef+width/2.0f, yRef, xRef+width/2.0f, yRef+height, lineWidth, defaultColor, painter);
    // Zero mark
    drawLine(xRef, yRef+height/2.0f, xRef+width, yRef+height/2.0f, lineWidth, defaultColor, painter);
    // Horizontal bottom line
    drawLine(xRef, yRef+height, xRef+width, yRef+height, lineWidth, defaultColor, painter);

    // Text
    QString label;
    label.sprintf("< %06.2f", value);
    paintText(label, defaultColor, 3.0f, xRef+width/2.0f, yRef+height-((scaledValue - minRate)/(maxRate-minRate))*height - 1.6f, painter);
}

void HDDisplay::drawGauge(float xRef, float yRef, float radius, float min, float max, QString name, float value, const QColor& color, QPainter* painter, bool symmetric, QPair<float, float> goodRange, QPair<float, float> criticalRange, bool solid)
{
    // Draw the circle
    QPen circlePen(Qt::SolidLine);

    // Rotate the whole gauge with this angle (in radians) for the zero position
    float zeroRotation;
    if (symmetric) {
        zeroRotation = 1.35f;
    } else {
        zeroRotation = 0.49f;
    }

    // Scale the rotation so that the gauge does one revolution
    // per max. change
    float rangeScale;
    if (symmetric) {
        rangeScale = ((2.0f * M_PI) / (max - min)) * 0.57f;
    } else {
        rangeScale = ((2.0f * M_PI) / (max - min)) * 0.72f;
    }

    const float scaledValue = (value-min)*rangeScale;

    float nameHeight = radius / 2.6f;
    paintText(name.toUpper(), color, nameHeight*0.7f, xRef-radius, yRef-radius, painter);

    // Ensure some space
    nameHeight *= 1.2f;

    if (!solid) {
        circlePen.setStyle(Qt::DotLine);
    }
    circlePen.setWidth(refLineWidthToPen(radius/12.0f));
    circlePen.setColor(color);

    if (symmetric) {
        circlePen.setStyle(Qt::DashLine);
    }
    painter->setBrush(Qt::NoBrush);
    painter->setPen(circlePen);
    drawCircle(xRef, yRef+nameHeight, radius, 0.0f, color, painter);
    //drawCircle(xRef, yRef+nameHeight, radius, 0.0f, 170.0f, 1.0f, color, painter);

    QString label;

    // Show integer values without decimal places
    if (intValues.contains(name)) {
        label.sprintf("% 05d", (int)value);
    } else {
        label.sprintf("% 06.1f", value);
    }


    // Text
    // height
    const float textHeight = radius/2.1f;
    const float textX = xRef-radius/3.0f;
    const float textY = yRef+radius/2.0f;

    // Draw background rectangle
    QBrush brush(QGC::colorBackground, Qt::SolidPattern);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);

    if (symmetric) {
        painter->drawRect(refToScreenX(xRef-radius), refToScreenY(yRef+nameHeight+radius/4.0f), refToScreenX(radius+radius), refToScreenY((radius - radius/4.0f)*1.2f));
    } else {
        painter->drawRect(refToScreenX(xRef-radius/2.5f), refToScreenY(yRef+nameHeight+radius/4.0f), refToScreenX(radius+radius/2.0f), refToScreenY((radius - radius/4.0f)*1.2f));
    }

    // Draw good value and crit. value markers
    if (goodRange.first != goodRange.second) {
        QRectF rectangle(refToScreenX(xRef-radius/2.0f), refToScreenY(yRef+nameHeight-radius/2.0f), refToScreenX(radius*2.0f), refToScreenX(radius*2.0f));
        painter->setPen(Qt::green);
        //int start = ((goodRange.first*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f;// + 16.0f * 60.0f;
        //int span  = start - ((goodRange.second*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f;
        //painter->drawArc(rectangle, start, span);
    }

    if (criticalRange.first != criticalRange.second) {
        QRectF rectangle(refToScreenX(xRef-radius/2.0f-3.0f), refToScreenY(yRef+nameHeight-radius/2.0f-3.0f), refToScreenX(radius*2.0f), refToScreenX(radius*2.0f));
        painter->setPen(Qt::yellow);
        //int start = ((criticalRange.first*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f - 180.0f*16.0f;// + 16.0f * 60.0f;
        //int span  = start - ((criticalRange.second*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f + 180.0f*16.0f;
        //painter->drawArc(rectangle, start, span);
    }

    // Draw the value
    //painter->setPen(textColor);
    paintText(label, QGC::colorCyan, textHeight, textX, textY+nameHeight, painter);
    //paintText(label, color, ((radius - radius/3.0f)*1.1f), xRef-radius/2.5f, yRef+radius/3.0f, painter);

    // Draw the needle

    const float maxWidth = radius / 6.0f;
    const float minWidth = maxWidth * 0.3f;

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef+nameHeight+radius * 0.05f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef+nameHeight+radius * 0.89f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef+nameHeight+radius * 0.89f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef+nameHeight+radius * 0.05f));
    p.replace(4, QPointF(xRef,               yRef+nameHeight+radius * 0.0f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef+nameHeight+radius * 0.05f));


    rotatePolygonClockWiseRad(p, scaledValue+zeroRotation, QPointF(xRef, yRef+nameHeight));

    QBrush indexBrush;
    indexBrush.setColor(color);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::NoPen);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}


void HDDisplay::drawSystemIndicator(float xRef, float yRef, int maxNum, float maxWidth, float maxHeight, QPainter* painter)
{
    if (values.size() > 0) {
        QString selectedKey = values.begin().key();
        //   | | | | | |
        //   | | | | | |
        //   x speed: 2.54

        // One column per value
        QMapIterator<QString, double> value(values);

        float x = xRef;
        float y = yRef;

        const float vspacing = 1.0f;
        float width = 1.5f;
        float height = 1.5f;
        const float hspacing = 0.6f;

        int i = 0;
        while (value.hasNext() && i < maxNum && x < maxWidth && y < maxHeight) {
            value.next();
            QBrush brush(Qt::SolidPattern);


            if (value.value() < 0.01f && value.value() > -0.01f) {
                brush.setColor(Qt::gray);
            } else if (value.value() > 0.01f) {
                brush.setColor(Qt::blue);
            } else {
                brush.setColor(Qt::yellow);
            }

            painter->setBrush(brush);
            painter->setPen(Qt::NoPen);

            // Draw current value colormap
            painter->drawRect(refToScreenX(x), refToScreenY(y), refToScreenX(width), refToScreenY(height));

            // Draw change rate colormap
            painter->drawRect(refToScreenX(x), refToScreenY(y+height+hspacing), refToScreenX(width), refToScreenY(height));

            // Draw mean value colormap
            painter->drawRect(refToScreenX(x), refToScreenY(y+2.0f*(height+hspacing)), refToScreenX(width), refToScreenY(height));

            // Add spacing
            x += width+vspacing;

            // Iterate
            i++;
        }

        // Draw detail label
        QString detail = "NO DATA AVAILABLE";

        if (values.contains(selectedKey)) {
            detail = values.find(selectedKey).key();
            detail.append(": ");
            detail.append(QString::number(values.find(selectedKey).value()));
        }
        paintText(detail, QColor(255, 255, 255), 3.0f, xRef, yRef+3.0f*(height+hspacing)+1.0f, painter);
    }
}

void HDDisplay::drawChangeIndicatorGauge(float xRef, float yRef, float radius, float expectedMaxChange, float value, const QColor& color, QPainter* painter, bool solid)
{
    // Draw the circle
    QPen circlePen(Qt::SolidLine);
    if (!solid) circlePen.setStyle(Qt::DotLine);
    circlePen.setWidth(refLineWidthToPen(0.5f));
    circlePen.setColor(defaultColor);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(circlePen);
    drawCircle(xRef, yRef, radius, 200.0f, color, painter);
    //drawCircle(xRef, yRef, radius, 200.0f, 170.0f, 1.0f, color, painter);

    QString label;
    label.sprintf("%05.1f", value);

    // Draw the value
    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);

    // Draw the needle
    // Scale the rotation so that the gauge does one revolution
    // per max. change
    const float rangeScale = (2.0f * M_PI) / expectedMaxChange;
    const float maxWidth = radius / 10.0f;
    const float minWidth = maxWidth * 0.3f;

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(4, QPointF(xRef,               yRef-radius * 0.46f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));

    rotatePolygonClockWiseRad(p, value*rangeScale, QPointF(xRef, yRef));

    QBrush indexBrush;
    indexBrush.setColor(defaultColor);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::SolidLine);
    painter->setPen(defaultColor);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}

/**
 * Paint text on top of the image and OpenGL drawings
 *
 * @param text chars to write
 * @param color text color
 * @param fontSize text size in mm
 * @param refX position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
 * @param refY position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
 */
void HDDisplay::paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter)
{
    QPen prevPen = painter->pen();
    float pPositionX = refToScreenX(refX) - (fontSize*scalingFactor*0.072f);
    float pPositionY = refToScreenY(refY) - (fontSize*scalingFactor*0.212f);

    QFont font("Bitstream Vera Sans");
    // Enforce minimum font size of 5 pixels
    int fSize = qMax(5, (int)(fontSize*scalingFactor*1.26f));
    font.setPixelSize(fSize);

    QFontMetrics metrics = QFontMetrics(font);
    int border = qMax(4, metrics.leading());
    QRect rect = metrics.boundingRect(0, 0, width() - 2*border, int(height()*0.125),
                                      Qt::AlignLeft | Qt::TextWordWrap, text);
    painter->setPen(color);
    painter->setFont(font);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->drawText(pPositionX, pPositionY,
                      rect.width(), rect.height(),
                      Qt::AlignCenter | Qt::TextWordWrap, text);
    painter->setPen(prevPen);
}

float HDDisplay::refLineWidthToPen(float line)
{
    return line * 2.50f;
}

// Connect a generic source
void HDDisplay::addSource(QObject* obj)
{
    //genericSources.append(obj);
    // FIXME XXX HACK
//    if (plots.size() > 0)
//    {
        connect(obj, SIGNAL(valueChanged(int,QString,QString,qint8,quint64)), this, SLOT(updateValue(int,QString,QString,qint8,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,quint8,quint64)), this, SLOT(updateValue(int,QString,QString,quint8,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,qint16,quint64)), this, SLOT(updateValue(int,QString,QString,qint16,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,quint16,quint64)), this, SLOT(updateValue(int,QString,QString,quint16,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,qint32,quint64)), this, SLOT(updateValue(int,QString,QString,qint32,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,quint32,quint64)), this, SLOT(updateValue(int,QString,QString,quint32,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,quint64,quint64)), this, SLOT(updateValue(int,QString,QString,quint64,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,qint64,quint64)), this, SLOT(updateValue(int,QString,QString,qint64,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,double,quint64)), this, SLOT(updateValue(int,QString,QString,double,quint64)));
//    }
}

// Disconnect a generic source
void HDDisplay::removeSource(QObject* obj)
{
    //genericSources.append(obj);
    // FIXME XXX HACK
//    if (plots.size() > 0)
//    {
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,qint8,quint64)), this, SLOT(updateValue(int,QString,QString,qint8,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,quint8,quint64)), this, SLOT(updateValue(int,QString,QString,quint8,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,qint16,quint64)), this, SLOT(updateValue(int,QString,QString,qint16,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,quint16,quint64)), this, SLOT(updateValue(int,QString,QString,quint16,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,qint32,quint64)), this, SLOT(updateValue(int,QString,QString,qint32,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,quint32,quint64)), this, SLOT(updateValue(int,QString,QString,quint32,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,quint64,quint64)), this, SLOT(updateValue(int,QString,QString,quint64,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,qint64,quint64)), this, SLOT(updateValue(int,QString,QString,qint64,quint64)));
        disconnect(obj, SIGNAL(valueChanged(int,QString,QString,double,quint64)), this, SLOT(updateValue(int,QString,QString,double,quint64)));
//    }
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const qint8 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const quint8 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const qint16 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const quint16 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const qint32 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const quint32 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec)
{
    if (!intValues.contains(name)) intValues.insert(name, true);
    updateValue(uasId, name, unit, (double)value, msec);
}

void HDDisplay::updateValue(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec)
{
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    // Update mean
    const float oldMean = valuesMean.value(name, 0.0f);
    const int meanCount = valuesCount.value(name, 0);
    valuesMean.insert(name, (oldMean * meanCount +  value) / (meanCount + 1));
    valuesCount.insert(name, meanCount + 1);
    valuesDot.insert(name, (value - values.value(name, 0.0f)) / ((msec - lastUpdate.value(name, 0))/1000.0f));
    if (values.value(name, 0.0) != value) valuesChanged = true;
    values.insert(name, value);
    units.insert(name, unit);
    lastUpdate.insert(name, msec);
}

/**
 * @param y coordinate in pixels to be converted to reference mm units
 * @return the screen coordinate relative to the QGLWindow origin
 */
float HDDisplay::refToScreenX(float x)
{
    return (scalingFactor * x);
}

/**
 * @param x coordinate in pixels to be converted to reference mm units
 * @return the screen coordinate relative to the QGLWindow origin
 */
float HDDisplay::refToScreenY(float y)
{
    return (scalingFactor * y);
}

float HDDisplay::screenToRefX(float x)
{
    return x/scalingFactor;
}

float HDDisplay::screenToRefY(float y)
{
    return y/scalingFactor;
}

void HDDisplay::drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter)
{
    QPen pen(Qt::SolidLine);
    pen.setWidth(refLineWidthToPen(width));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(QPoint(refToScreenX(refX1), refToScreenY(refY1)), QPoint(refToScreenX(refX2), refToScreenY(refY2)));
}

void HDDisplay::drawEllipse(float refX, float refY, float radiusX, float radiusY, float lineWidth, const QColor& color, QPainter* painter)
{
    QPen pen(painter->pen().style());
    pen.setWidth(refLineWidthToPen(lineWidth));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawEllipse(QPointF(refToScreenX(refX), refToScreenY(refY)), refToScreenX(radiusX), refToScreenY(radiusY));
}

void HDDisplay::drawCircle(float refX, float refY, float radius, float lineWidth, const QColor& color, QPainter* painter)
{
    drawEllipse(refX, refY, radius, radius, lineWidth, color, painter);
}

void HDDisplay::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void HDDisplay::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    refreshTimer->start(updateInterval);
}

void HDDisplay::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    refreshTimer->stop();
    saveState();
}


///**
//  * Sets the current centerpoint.  Also updates the scene's center point.
//  * Unlike centerOn, which has no way of getting the floating point center
//  * back, SetCenter() stores the center point.  It also handles the special
//  * sidebar case.  This function will claim the centerPoint to sceneRec ie.
//  * the centerPoint must be within the sceneRec.
//  */
////Set the current centerpoint in the
//void HDDisplay::setCenter(const QPointF& centerPoint) {
//    //Get the rectangle of the visible area in scene coords
//    QRectF visibleArea = mapToScene(rect()).boundingRect();
//
//    //Get the scene area
//    QRectF sceneBounds = sceneRect();
//
//    double boundX = visibleArea.width() / 2.0;
//    double boundY = visibleArea.height() / 2.0;
//    double boundWidth = sceneBounds.width() - 2.0 * boundX;
//    double boundHeight = sceneBounds.height() - 2.0 * boundY;
//
//    //The max boundary that the centerPoint can be to
//    QRectF bounds(boundX, boundY, boundWidth, boundHeight);
//
//    if(bounds.contains(centerPoint)) {
//        //We are within the bounds
//        currentCenterPoint = centerPoint;
//    } else {
//        //We need to clamp or use the center of the screen
//        if(visibleArea.contains(sceneBounds)) {
//            //Use the center of scene ie. we can see the whole scene
//            currentCenterPoint = sceneBounds.center();
//        } else {
//
//            currentCenterPoint = centerPoint;
//
//            //We need to clamp the center. The centerPoint is too large
//            if(centerPoint.x() > bounds.x() + bounds.width()) {
//                currentCenterPoint.setX(bounds.x() + bounds.width());
//            } else if(centerPoint.x() < bounds.x()) {
//                currentCenterPoint.setX(bounds.x());
//            }
//
//            if(centerPoint.y() > bounds.y() + bounds.height()) {
//                currentCenterPoint.setY(bounds.y() + bounds.height());
//            } else if(centerPoint.y() < bounds.y()) {
//                currentCenterPoint.setY(bounds.y());
//            }
//
//        }
//    }
//
//    //Update the scrollbars
//    centerOn(currentCenterPoint);
//}
//
///**
//  * Handles when the mouse button is pressed
//  */
//void HDDisplay::mousePressEvent(QMouseEvent* event) {
//    //For panning the view
//    lastPanPoint = event->pos();
//    setCursor(Qt::ClosedHandCursor);
//}
//
///**
//  * Handles when the mouse button is released
//  */
//void HDDisplay::mouseReleaseEvent(QMouseEvent* event) {
//    setCursor(Qt::OpenHandCursor);
//    lastPanPoint = QPoint();
//}
//
///**
//*Handles the mouse move event
//*/
//void HDDisplay::mouseMoveEvent(QMouseEvent* event) {
//    if(!lastPanPoint.isNull()) {
//        //Get how much we panned
//        QPointF delta = mapToScene(lastPanPoint) - mapToScene(event->pos());
//        lastPanPoint = event->pos();
//
//        //Update the center ie. do the pan
//        setCenter(getCenter() + delta);
//    }
//}
//
///**
//  * Zoom the view in and out.
//  */
//void HDDisplay::wheelEvent(QWheelEvent* event) {
//
//    //Get the position of the mouse before scaling, in scene coords
//    QPointF pointBeforeScale(mapToScene(event->pos()));
//
//    //Get the original screen centerpoint
//    QPointF screenCenter = getCenter(); //CurrentCenterPoint; //(visRect.center());
//
//    //Scale the view ie. do the zoom
//    double scaleFactor = 1.15; //How fast we zoom
//    if(event->delta() > 0) {
//        //Zoom in
//        scale(scaleFactor, scaleFactor);
//    } else {
//        //Zooming out
//        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
//    }
//
//    //Get the position after scaling, in scene coords
//    QPointF pointAfterScale(mapToScene(event->pos()));
//
//    //Get the offset of how the screen moved
//    QPointF offset = pointBeforeScale - pointAfterScale;
//
//    //Adjust to the new center for correct zooming
//    QPointF newCenter = screenCenter + offset;
//    setCenter(newCenter);
//}
//
///**
//  * Need to update the center so there is no jolt in the
//  * interaction after resizing the widget.
//  */
//void HDDisplay::resizeEvent(QResizeEvent* event) {
//    //Get the rectangle of the visible area in scene coords
//    QRectF visibleArea = mapToScene(rect()).boundingRect();
//    setCenter(visibleArea.center());
//
//    //Call the subclass resize so the scrollbars are updated correctly
//    QGraphicsView::resizeEvent(event);
//}

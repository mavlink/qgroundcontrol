#include <QtGui>
#include <QSettings>

#include "HUD2Drawer.h"
#include "HUD2IndicatorHorizon.h"
#include "HUD2Math.h"

HUD2IndicatorHorizon::HUD2IndicatorHorizon(const double *pitch,
                                           const double *roll, QWidget *parent) :
    QWidget(parent),
    pitchline(&this->gap, this),
    crosshair(&this->gap, this),
    pitch(pitch),
    roll(roll)
{
    QColor color;
    QSettings settings;
    settings.beginGroup("QGC_HUD2");

    this->gap = 6;

    bigScratchLenStep = settings.value("HORIZON_BIG_SCRATCH_LEN_STEP", 20.0).toDouble();
    bigScratchValueStep = settings.value("HORIZON_BIG_SCRATCH_VALUE_STEP", 10).toInt();
    stepsBig = settings.value("HORIZON_STEPS_BIG", 4).toInt();

    color = settings.value("INSTRUMENTS_COLOR", INSTRUMENTS_COLOR_DEFAULT).value<QColor>();
    this->pen.setColor(color);
    pitchline.setColor(color);
    crosshair.setColor(color);

    color = settings.value("SKY_COLOR", SKY_COLOR_DEFAULT).value<QColor>();
    skyPen   = QPen(color);
    skyBrush = QBrush(color);

    color = settings.value("GND_COLOR", GND_COLOR_DEFAULT).value<QColor>();
    gndPen   = QPen(color);
    gndBrush = QBrush(color);

    coloredBackground = settings.value("HORIZON_COLORED_BG", true).toBool();;

    settings.endGroup();
}

void HUD2IndicatorHorizon::updateGeometry(const QSize &size){
    this->size_cached = size;

    int a = percent2pix_w(size, this->gap);

    // wings
    int x1 = size.width() / 2;
    int tmp = percent2pix_h(size, 1);
    tmp = qBound(2, tmp, 10);
    pen.setWidth(tmp);
    hirizonleft.setLine(-x1, 0, -a, 0);
    horizonright.setLine(a, 0, x1, 0);

    // pitchlines
    big_pixstep = percent2pix_hF(size, bigScratchLenStep);
    pitchline.updateGeometry(size);

    // crosshair
    crosshair.updateGeometry(size);
}

/**
 * @brief drawpitchlines
 * @param painter
 * @param degstep
 * @param pixstep
 */
void HUD2IndicatorHorizon::drawpitchlines(QPainter *painter, qreal degstep, qreal pixstep){
    int i = 1;

    painter->save();
    for (i=1; i<=stepsBig; i++){
        painter->translate(0, pixstep);
        pitchline.paint(painter, -i * degstep);
    }
    painter->restore();

    painter->save();
    for (i=1; i<=stepsBig; i++){
        painter->translate(0, -pixstep);
        pitchline.paint(painter, i * degstep);
    }
    painter->restore();
}

static int _getline_y(QPoint p1, QPoint p2, int x){
    int x1 = p1.rx();
    int y1 = p1.ry();
    int x2 = p2.rx();
    int y2 = p2.ry();

    return ((x2*y1 - x1*y2) + x * (y2 - y1)) / (x2 -x1);
}

void HUD2IndicatorHorizon::paint(QPainter *painter){

    qreal pitch_ = rad2deg(-*pitch);
    qreal delta_y = pitch_ * (big_pixstep / bigScratchValueStep);
    qreal delta_x = tan(-*roll) * delta_y;

    // create complex transfomation
    QPoint center = painter->window().center();
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.translate(delta_x, delta_y);
    transform.rotate(rad2deg(*roll));

    if (coloredBackground){
        // draw colored background
        /* some kind of hack to create poligon with minimal needed area:
         * - create rectangle
         * - apply transform to it
         * - from output polygon got points laying on horizon line
         * - use line formulae to calculate points of new polygon
         */

        painter->save();
        QRect rect = QRect(QPoint(-1000,0), QPoint(1000,1000));
        QPolygon poly = transform.mapToPolygon(rect);

        int x = 0;
        int w = painter->window().width();
        int h = painter->window().height();
        QPoint point_left = QPoint(x, _getline_y(poly.point(0), poly.point(1), x));
        x = w;
        QPoint point_right = QPoint(x, _getline_y(poly.point(0), poly.point(1), x));

        poly.setPoint(0, point_left);
        poly.setPoint(1, point_right);
        poly.setPoint(2, w, 0);
        poly.setPoint(3, 0, 0);

        painter->setBrush(skyBrush);
        painter->setPen(skyPen);
        painter->drawPolygon(poly);

        poly.setPoint(2, w, h);
        poly.setPoint(3, 0, h);
        painter->setBrush(gndBrush);
        painter->setPen(gndPen);
        painter->drawPolygon(poly);

        painter->restore();
    }

    // draw other stuff
    painter->save();
    painter->setTransform(transform);

    // pitchlines
    this->drawpitchlines(painter, bigScratchValueStep, big_pixstep);

    // horizon lines
    painter->setPen(pen);
    painter->drawLine(hirizonleft);
    painter->drawLine(horizonright);

    painter->restore();

    // central cross
    crosshair.paint(painter);
}

void HUD2IndicatorHorizon::setColor(QColor color){
    pen.setColor(color);
    pitchline.setColor(color);
    crosshair.setColor(color);
}

void HUD2IndicatorHorizon::setSkyColor(QColor color){
    skyPen.setColor(color);
    skyBrush.setColor(color);
}

void HUD2IndicatorHorizon::setGndColor(QColor color){
    gndPen.setColor(color);
    gndBrush.setColor(color);
}

void HUD2IndicatorHorizon::setColoredBg(bool checked){
    coloredBackground = checked;
}

void HUD2IndicatorHorizon::setBigScratchLenStep(double value){
    bigScratchLenStep = value;
    updateGeometry(size_cached);
}

void HUD2IndicatorHorizon::setBigScratchValueStep(int value){
    bigScratchValueStep = value;
    updateGeometry(size_cached);
}

void HUD2IndicatorHorizon::setStepsBig(int value){
    stepsBig = value;
    updateGeometry(size_cached);
}

void HUD2IndicatorHorizon::syncSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_HUD2");

    settings.setValue("HORIZON_STEPS_BIG", stepsBig);
    settings.setValue("HORIZON_BIG_SCRATCH_VALUE_STEP", bigScratchValueStep);
    settings.setValue("HORIZON_BIG_SCRATCH_LEN_STEP", bigScratchLenStep);
    settings.setValue("HORIZON_COLORED_BG", coloredBackground);

    settings.endGroup();
}

HUD2FormHorizon *HUD2IndicatorHorizon::getForm(void){
    form = new HUD2FormHorizon(this);

    form->ui->checkBox->setChecked(coloredBackground);
    connect(form->ui->checkBox, SIGNAL(toggled(bool)), this, SLOT(setColoredBg(bool)));

    form->ui->bigScratchLenStep->setRange(1, 100);
    form->ui->bigScratchLenStep->setSingleStep(0.5);
    form->ui->bigScratchLenStep->setValue(bigScratchLenStep);
    connect(form->ui->bigScratchLenStep, SIGNAL(valueChanged(double)),
            this, SLOT(setBigScratchLenStep(double)));

    form->ui->bigScratchValueStep->setRange(1, 10000);
    form->ui->bigScratchValueStep->setValue(bigScratchValueStep);
    connect(form->ui->bigScratchValueStep, SIGNAL(valueChanged(int)),
            this, SLOT(setBigScratchValueStep(int)));

    form->ui->stepsBig->setRange(1, 10);
    form->ui->stepsBig->setSingleStep(1);
    form->ui->stepsBig->setValue(stepsBig);
    connect(form->ui->stepsBig, SIGNAL(valueChanged(int)),
            this, SLOT(setStepsBig(int)));

    connect(form, SIGNAL(destroyed()), this, SLOT(syncSettings()));

    return form;
}

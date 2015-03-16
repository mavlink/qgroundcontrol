#include "PrimaryFlightDisplay.h"
#include "UASManager.h"

#include <QDebug>
#include <QRectF>
#include <cmath>
#include <QPen>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QtCore/qmath.h>

static const float LINEWIDTH = 0.0036f;
static const float SMALL_TEXT_SIZE = 0.028f;
static const float MEDIUM_TEXT_SIZE = SMALL_TEXT_SIZE*1.2f;
static const float LARGE_TEXT_SIZE = MEDIUM_TEXT_SIZE*1.2f;

static const bool SHOW_ZERO_ON_SCALES = true;

// all in units of display height
static const float ROLL_SCALE_RADIUS = 0.42f;
static const float ROLL_SCALE_TICKMARKLENGTH = 0.04f;
static const float ROLL_SCALE_MARKERWIDTH = 0.06f;
static const float ROLL_SCALE_MARKERHEIGHT = 0.04f;
// scale max. degrees
static const int ROLL_SCALE_RANGE = 60;

// fraction of height to translate for each degree of pitch.
static const float PITCHTRANSLATION = 65;
// 5 degrees for each line
static const int PITCH_SCALE_RESOLUTION = 5;
static const float PITCH_SCALE_MAJORWIDTH = 0.1f;
static const float PITCH_SCALE_MINORWIDTH = 0.066f;

// Beginning from PITCH_SCALE_WIDTHREDUCTION_FROM degrees of +/- pitch, the
// width of the lines is reduced, down to PITCH_SCALE_WIDTHREDUCTION times
// the normal width. This helps keep orientation in extreme attitudes.
static const int PITCH_SCALE_WIDTHREDUCTION_FROM = 30;
static const float PITCH_SCALE_WIDTHREDUCTION = 0.3f;

static const int PITCH_SCALE_HALFRANGE = 15;

// The number of degrees to either side of the heading to draw the compass disk.
// 180 is valid, this will draw a complete disk. If the disk is partly clipped
// away, less will do.

static const int  COMPASS_DISK_MAJORTICK = 10;
static const int  COMPASS_DISK_ARROWTICK = 45;
static const int  COMPASS_DISK_RESOLUTION = 10;
static const float COMPASS_DISK_MARKERWIDTH = 0.2f;
static const float COMPASS_DISK_MARKERHEIGHT = 0.133f;

static const int  CROSSTRACK_MAX = 1000;
static const float CROSSTRACK_RADIUS = 0.6f;

static const float TAPE_GAUGES_TICKWIDTH_MAJOR = 0.25f;
static const float TAPE_GAUGES_TICKWIDTH_MINOR = 0.15f;

// The altitude difference between top and bottom of scale
static const int ALTIMETER_LINEAR_SPAN = 50;
// every 5 meters there is a tick mark
static const int ALTIMETER_LINEAR_RESOLUTION = 5;
// every 10 meters there is a number
static const int ALTIMETER_LINEAR_MAJOR_RESOLUTION = 10;

// min. and max. vertical velocity
static const int ALTIMETER_VVI_SPAN = 5;
static const float ALTIMETER_VVI_WIDTH = 0.2f;

// Now the same thing for airspeed!
static const int AIRSPEED_LINEAR_SPAN = 15;
static const int AIRSPEED_LINEAR_RESOLUTION = 1;
static const int AIRSPEED_LINEAR_MAJOR_RESOLUTION = 5;

/*
 *@TODO:
 * global fixed pens (and painters too?)
 * repaint on demand multiple canvases
 * multi implementation with shared model class
 */
double PrimaryFlightDisplay_round(double value, int digits=0)
{
    return floor(value * pow(10.0, digits) + 0.5) / pow(10.0, digits);
}

qreal PrimaryFlightDisplay_constrain(qreal value, qreal min, qreal max) {
    if (value<min) value=min;
    else if(value>max) value=max;
    return value;
}

const int PrimaryFlightDisplay::tickValues[] = {10, 20, 30, 45, 60};
const QString PrimaryFlightDisplay::compassWindNames[] = {
    QString("N"),
    QString("NE"),
    QString("E"),
    QString("SE"),
    QString("S"),
    QString("SW"),
    QString("W"),
    QString("NW")
};

PrimaryFlightDisplay::PrimaryFlightDisplay(QWidget *parent) :
    QWidget(parent),

    _valuesChanged(false),
    _valuesLastPainted(QGC::groundTimeMilliseconds()),

    uas(NULL),

    roll(0),
    pitch(0),
    heading(0),

    altitudeAMSL(std::numeric_limits<double>::quiet_NaN()),
    altitudeWGS84(std::numeric_limits<double>::quiet_NaN()),
    altitudeRelative(std::numeric_limits<double>::quiet_NaN()),

    groundSpeed(std::numeric_limits<double>::quiet_NaN()),
    airSpeed(std::numeric_limits<double>::quiet_NaN()),
    climbRate(std::numeric_limits<double>::quiet_NaN()),

    navigationCrosstrackError(0),
    navigationTargetBearing(std::numeric_limits<double>::quiet_NaN()),

    layout(COMPASS_INTEGRATED),
    style(OVERLAY_HSI),

    redColor(QColor::fromHsvF(0, 0.75, 0.9)),
    amberColor(QColor::fromHsvF(0.12, 0.6, 1.0)),
    greenColor(QColor::fromHsvF(0.25, 0.8, 0.8)),

    lineWidth(2),
    fineLineWidth(1),

    instrumentEdgePen(QColor::fromHsvF(0, 0, 0.65, 0.5)),
    instrumentBackground(QColor::fromHsvF(0, 0, 0.3, 0.3)),
    instrumentOpagueBackground(QColor::fromHsvF(0, 0, 0.3, 1.0)),

    font("Bitstream Vera Sans"),
    refreshTimer(new QTimer(this))
{
    setMinimumSize(120, 80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Connect with UAS signal
    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)), this, SLOT(forgetUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    // Refresh timer
    refreshTimer->setInterval(updateInterval);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
}

PrimaryFlightDisplay::~PrimaryFlightDisplay()
{
    refreshTimer->stop();
}

QSize PrimaryFlightDisplay::sizeHint() const
{
    return QSize(width(), (int)(width() * 3.0f / 4.0f));
}


void PrimaryFlightDisplay::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display) events
    QWidget::showEvent(event);
    refreshTimer->start(updateInterval);
    emit visibilityChanged(true);
}

void PrimaryFlightDisplay::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display) events
    refreshTimer->stop();
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void PrimaryFlightDisplay::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);

    qreal size = e->size().width();

    lineWidth = PrimaryFlightDisplay_constrain(size*LINEWIDTH, 1, 6);
    fineLineWidth = PrimaryFlightDisplay_constrain(size*LINEWIDTH*2/3, 1, 2);

    instrumentEdgePen.setWidthF(fineLineWidth);

    smallTextSize = size * SMALL_TEXT_SIZE;
    mediumTextSize = size * MEDIUM_TEXT_SIZE;
    largeTextSize = size * LARGE_TEXT_SIZE;
}

void PrimaryFlightDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    doPaint();
}

void PrimaryFlightDisplay::checkUpdate()
{
    if (uas && (_valuesChanged || (QGC::groundTimeMilliseconds() - _valuesLastPainted) > 260)) {
        update();
        _valuesChanged = false;
        _valuesLastPainted = QGC::groundTimeMilliseconds();
    }
}

void PrimaryFlightDisplay::forgetUAS(UASInterface* uas)
{
    if (this->uas != NULL && this->uas == uas) {
        // Disconnect any previously connected active MAV
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(speedChanged(UASInterface*, double, double, quint64)), this, SLOT(updateSpeed(UASInterface*, double, double, quint64)));
        disconnect(this->uas, SIGNAL(altitudeChanged(UASInterface*, double, double, double, double, quint64)), this, SLOT(updateAltitude(UASInterface*, double, double, double, quint64)));
        disconnect(this->uas, SIGNAL(navigationControllerErrorsChanged(UASInterface*, double, double, double)), this, SLOT(updateNavigationControllerErrors(UASInterface*, double, double, double)));
    }
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void PrimaryFlightDisplay::setActiveUAS(UASInterface* uas)
{
    if (uas == this->uas)
        return; //no need to rewire

    // Disconnect the previous one (if any)
    forgetUAS(this->uas);

    if (uas) {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,int,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(uas, SIGNAL(speedChanged(UASInterface*, double, double, quint64)), this, SLOT(updateSpeed(UASInterface*, double, double, quint64)));
        connect(uas, SIGNAL(altitudeChanged(UASInterface*, double, double, double, double, quint64)), this, SLOT(updateAltitude(UASInterface*, double, double, double, double, quint64)));
        connect(uas, SIGNAL(navigationControllerErrorsChanged(UASInterface*, double, double, double)), this, SLOT(updateNavigationControllerErrors(UASInterface*, double, double, double)));

        // Set new UAS
        this->uas = uas;
    }
}

void PrimaryFlightDisplay::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);

        if (isinf(roll)) {
            this->roll = std::numeric_limits<double>::quiet_NaN();
        } else {

            float rolldeg = roll * (180.0 / M_PI);

            if (fabsf(roll - rolldeg) > 2.5f) {
                _valuesChanged = true;
            }

            this->roll = rolldeg;
        }

        if (isinf(pitch)) {
            this->pitch = std::numeric_limits<double>::quiet_NaN();
        } else {

            float pitchdeg = pitch * (180.0 / M_PI);

            if (fabsf(pitch - pitchdeg) > 2.5f) {
                _valuesChanged = true;
            }

            this->pitch = pitchdeg;
        }

        if (isinf(yaw)) {
            this->heading = std::numeric_limits<double>::quiet_NaN();
        } else {

            yaw = yaw * (180.0 / M_PI);
            if (yaw<0) yaw+=360;

            if (fabsf(heading - yaw) > 10.0f) {
                _valuesChanged = true;
            }

            this->heading = yaw;
        }

}

void PrimaryFlightDisplay::updateAttitude(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(component);
    this->updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void PrimaryFlightDisplay::updateSpeed(UASInterface* uas, double _groundSpeed, double _airSpeed, quint64 timestamp)
{
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);

    if (fabsf(groundSpeed - _groundSpeed) > 0.5f) {
        _valuesChanged = true;
    }

    if (fabsf(airSpeed - _airSpeed) > 1.0f) {
        _valuesChanged = true;
    }

    groundSpeed = _groundSpeed;
    airSpeed = _airSpeed;
}

void PrimaryFlightDisplay::updateAltitude(UASInterface* uas, double _altitudeAMSL, double _altitudeWGS84, double _altitudeRelative, double _climbRate, quint64 timestamp) {
    Q_UNUSED(uas);
    Q_UNUSED(timestamp);

    if (fabsf(altitudeAMSL - _altitudeAMSL) > 0.5f) {
        _valuesChanged = true;
    }

    if (fabsf(altitudeWGS84 - _altitudeWGS84) > 0.5f) {
        _valuesChanged = true;
    }

    if (fabsf(altitudeRelative - _altitudeRelative) > 0.5f) {
        _valuesChanged = true;
    }

    if (fabsf(climbRate - _climbRate) > 0.5f) {
        _valuesChanged = true;
    }

    altitudeAMSL = _altitudeAMSL;
    altitudeWGS84 = _altitudeWGS84;
    altitudeRelative = _altitudeRelative;
    climbRate = _climbRate;
}

void PrimaryFlightDisplay::updateNavigationControllerErrors(UASInterface* uas, double altitudeError, double speedError, double xtrackError) {
    Q_UNUSED(uas);
    this->navigationAltitudeError = altitudeError;
    this->navigationSpeedError = speedError;
    this->navigationCrosstrackError = xtrackError;
}


/*
 * Private and such
 */

// TODO: Move to UAS. Real working implementation.
bool PrimaryFlightDisplay::isAirplane() {
    if (!this->uas)
        return false;
    switch(this->uas->getSystemType()) {
    case MAV_TYPE_GENERIC:
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_AIRSHIP:
    case MAV_TYPE_FLAPPING_WING:
        return true;
    default:
        return false;
    }
}

// TODO: Implement. Should return true when navigating.
// That would be (APM) in AUTO and RTL modes.
// This could forward to a virtual on UAS bool isNavigatingAutonomusly() or whatever.
bool PrimaryFlightDisplay::shouldDisplayNavigationData() {
    return true;
}

void PrimaryFlightDisplay::drawTextCenter (
        QPainter& painter,
        QString text,
        float pixelSize,
        float x,
        float y)
{
    font.setPixelSize(pixelSize);
    painter.setFont(font);

    QFontMetrics metrics = QFontMetrics(font);
    QRect bounds = metrics.boundingRect(text);
    int flags = Qt::AlignCenter |  Qt::TextDontClip; // For some reason the bounds rect is too small!
    painter.drawText(x - bounds.width()/2, y - bounds.height()/2, bounds.width(), bounds.height(), flags, text);
}

void PrimaryFlightDisplay::drawTextLeftCenter (
        QPainter& painter,
        QString text,
        float pixelSize,
        float x,
        float y)
{
    font.setPixelSize(pixelSize);
    painter.setFont(font);

    QFontMetrics metrics = QFontMetrics(font);
    QRect bounds = metrics.boundingRect(text);
    int flags = Qt::AlignLeft | Qt::TextDontClip; // For some reason the bounds rect is too small!
    painter.drawText(x, y - bounds.height()/2, bounds.width(), bounds.height(), flags, text);
}

void PrimaryFlightDisplay::drawTextRightCenter (
        QPainter& painter,
        QString text,
        float pixelSize,
        float x,
        float y)
{
    font.setPixelSize(pixelSize);
    painter.setFont(font);

    QFontMetrics metrics = QFontMetrics(font);
    QRect bounds = metrics.boundingRect(text);
    int flags = Qt::AlignRight | Qt::TextDontClip; // For some reason the bounds rect is too small!
    painter.drawText(x - bounds.width(), y - bounds.height()/2, bounds.width(), bounds.height(), flags, text);
}

void PrimaryFlightDisplay::drawTextCenterTop (
        QPainter& painter,
        QString text,
        float pixelSize,
        float x,
        float y)
{
    font.setPixelSize(pixelSize);
    painter.setFont(font);

    QFontMetrics metrics = QFontMetrics(font);
    QRect bounds = metrics.boundingRect(text);
    int flags = Qt::AlignCenter | Qt::TextDontClip; // For some reason the bounds rect is too small!
    painter.drawText(x - bounds.width()/2, y+bounds.height(), bounds.width(), bounds.height(), flags, text);
}

void PrimaryFlightDisplay::drawTextCenterBottom (
        QPainter& painter,
        QString text,
        float pixelSize,
        float x,
        float y)
{
    font.setPixelSize(pixelSize);
    painter.setFont(font);

    QFontMetrics metrics(font);
    QRect bounds = metrics.boundingRect(text);
    int flags = Qt::AlignCenter;
    painter.drawText(x - bounds.width()/2, y, bounds.width(), bounds.height(), flags, text);
}

void PrimaryFlightDisplay::drawInstrumentBackground(QPainter& painter, QRectF edge) {
    painter.setPen(instrumentEdgePen);
    painter.drawRect(edge);
}

void PrimaryFlightDisplay::fillInstrumentBackground(QPainter& painter, QRectF edge) {
    painter.setPen(instrumentEdgePen);
    painter.setBrush(instrumentBackground);
    painter.drawRect(edge);
    painter.setBrush(Qt::NoBrush);
}

void PrimaryFlightDisplay::fillInstrumentOpagueBackground(QPainter& painter, QRectF edge) {
    painter.setPen(instrumentEdgePen);
    painter.setBrush(instrumentOpagueBackground);
    painter.drawRect(edge);
    painter.setBrush(Qt::NoBrush);
}

qreal pitchAngleToTranslation(qreal viewHeight, float pitch) {
    if (isnan(pitch))
        return 0;
    return pitch * viewHeight / PITCHTRANSLATION;
}

void PrimaryFlightDisplay::drawAIAirframeFixedFeatures(QPainter& painter, QRectF area) {
    // red line from -7/10 to -5/10 half-width
    // red line from 7/10 to 5/10 half-width
    // red slanted line from -2/10 half-width to 0
    // red slanted line from 2/10 half-width to 0
    // red arrow thing under roll scale
    // prepareTransform(painter, width, height);
    painter.resetTransform();
    painter.translate(area.center());

    qreal w = area.width();
    qreal h = area.height();

    QPen pen;
    pen.setWidthF(lineWidth * 1.5f);
    pen.setColor(redColor);
    painter.setPen(pen);

    float length = 0.15f;
    float side = 0.5f;
    // The 2 lines at sides.
    painter.drawLine(QPointF(-side*w, 0), QPointF(-(side-length)*w, 0));
    painter.drawLine(QPointF(side*w, 0), QPointF((side-length)*w, 0));

    float rel = length/qSqrt(2.0f);
    // The gull
    painter.drawLine(QPointF(rel*w, rel*w/2), QPoint(0, 0));
    painter.drawLine(QPointF(-rel*w, rel*w/2), QPoint(0, 0));

    // The roll scale marker.
    QPainterPath markerPath(QPointF(0, -w*ROLL_SCALE_RADIUS+1));
    markerPath.lineTo(-h*ROLL_SCALE_MARKERWIDTH/2, -w*(ROLL_SCALE_RADIUS-ROLL_SCALE_MARKERHEIGHT)+1);
    markerPath.lineTo(h*ROLL_SCALE_MARKERWIDTH/2, -w*(ROLL_SCALE_RADIUS-ROLL_SCALE_MARKERHEIGHT)+1);
    markerPath.closeSubpath();
    painter.drawPath(markerPath);
}

inline qreal min4(qreal a, qreal b, qreal c, qreal d) {
    if(b<a) a=b;
    if(c<a) a=c;
    if(d<a) a=d;
    return a;
}

inline qreal max4(qreal a, qreal b, qreal c, qreal d) {
    if(b>a) a=b;
    if(c>a) a=c;
    if(d>a) a=d;
    return a;
}

void PrimaryFlightDisplay::drawAIGlobalFeatures(
        QPainter& painter,
        QRectF mainArea,
        QRectF paintArea) {

    float displayRoll = this->roll;
    if (isnan(displayRoll))
        displayRoll = 0;

    painter.resetTransform();
    painter.translate(mainArea.center());

    qreal pitchPixels = pitchAngleToTranslation(mainArea.height(), pitch);
    qreal gradientEnd = pitchAngleToTranslation(mainArea.height(), 60);

    painter.rotate(-displayRoll);
    painter.translate(0, pitchPixels);

    // Calculate the radius of area we need to paint to cover all.
    QTransform rtx = painter.transform().inverted();

    QPointF topLeft = rtx.map(paintArea.topLeft());
    QPointF topRight = rtx.map(paintArea.topRight());
    QPointF bottomLeft = rtx.map(paintArea.bottomLeft());
    QPointF bottomRight = rtx.map(paintArea.bottomRight());

    // Just KISS... make a rectangluar basis.
    qreal minx = min4(topLeft.x(), topRight.x(), bottomLeft.x(), bottomRight.x());
    qreal maxx = max4(topLeft.x(), topRight.x(), bottomLeft.x(), bottomRight.x());
    qreal miny = min4(topLeft.y(), topRight.y(), bottomLeft.y(), bottomRight.y());
    qreal maxy = max4(topLeft.y(), topRight.y(), bottomLeft.y(), bottomRight.y());

    QPointF hzonLeft = QPoint(minx, 0);
    QPointF hzonRight = QPoint(maxx, 0);

    QPainterPath skyPath(hzonLeft);
    skyPath.lineTo(QPointF(minx, miny));
    skyPath.lineTo(QPointF(maxx, miny));
    skyPath.lineTo(hzonRight);
    skyPath.closeSubpath();

    QLinearGradient skyGradient(0, -gradientEnd, 0, 0);
    skyGradient.setColorAt(0, QColor::fromHsvF(0.6, 1.0, 0.7));
    skyGradient.setColorAt(1, QColor::fromHsvF(0.6, 0.25, 0.9));
    QBrush skyBrush(skyGradient);
    painter.fillPath(skyPath, skyBrush);

    QPainterPath groundPath(hzonRight);
    groundPath.lineTo(maxx, maxy);
    groundPath.lineTo(minx, maxy);
    groundPath.lineTo(hzonLeft);
    groundPath.closeSubpath();

    QLinearGradient groundGradient(0, gradientEnd, 0, 0);
    groundGradient.setColorAt(0, QColor::fromHsvF(0.25, 1, 0.5));
    groundGradient.setColorAt(1, QColor::fromHsvF(0.25, 0.25, 0.5));
    QBrush groundBrush(groundGradient);
    painter.fillPath(groundPath, groundBrush);

    QPen pen;
    pen.setWidthF(lineWidth);
    pen.setColor(greenColor);
    painter.setPen(pen);

    QPointF start(-mainArea.width(), 0);
    QPoint end(mainArea.width(), 0);
    painter.drawLine(start, end);
}

void PrimaryFlightDisplay::drawPitchScale(
        QPainter& painter,
        QRectF area,
        float intrusion,
        bool drawNumbersLeft,
        bool drawNumbersRight
        ) {

    Q_UNUSED(intrusion);
    
    float displayPitch = this->pitch;
    if (isnan(displayPitch))
        displayPitch = 0;

    // The area should be quadratic but if not width is the major size.
    qreal w = area.width();
    if (w<area.height()) w = area.height();

    QPen pen;
    pen.setWidthF(lineWidth);
    pen.setColor(Qt::white);
    painter.setPen(pen);

    QTransform savedTransform = painter.transform();

    // find the mark nearest center
    int snap = qRound((double)(displayPitch/PITCH_SCALE_RESOLUTION))*PITCH_SCALE_RESOLUTION;
    int _min = snap-PITCH_SCALE_HALFRANGE;
    int _max = snap+PITCH_SCALE_HALFRANGE;
    for (int degrees=_min; degrees<=_max; degrees+=PITCH_SCALE_RESOLUTION) {
        bool isMajor = degrees % (PITCH_SCALE_RESOLUTION*2) == 0;
        float linewidth =  isMajor ? PITCH_SCALE_MAJORWIDTH : PITCH_SCALE_MINORWIDTH;
        if (abs(degrees) > PITCH_SCALE_WIDTHREDUCTION_FROM) {
            // we want: 1 at PITCH_SCALE_WIDTHREDUCTION_FROM and PITCH_SCALE_WIDTHREDUCTION at 90.
            // That is PITCH_SCALE_WIDTHREDUCTION + (1-PITCH_SCALE_WIDTHREDUCTION) * f(pitch)
            // where f(90)=0 and f(PITCH_SCALE_WIDTHREDUCTION_FROM)=1
            // f(p) = (90-p) * 1/(90-PITCH_SCALE_WIDTHREDUCTION_FROM)
            // or PITCH_SCALE_WIDTHREDUCTION + f(pitch) - f(pitch) * PITCH_SCALE_WIDTHREDUCTION
            // or PITCH_SCALE_WIDTHREDUCTION (1-f(pitch)) + f(pitch)
            int fromVertical = abs(pitch>=0 ? 90-pitch : -90-pitch);
            float temp = fromVertical * 1/(90.0f-PITCH_SCALE_WIDTHREDUCTION_FROM);
            linewidth *= (PITCH_SCALE_WIDTHREDUCTION * (1-temp) + temp);
        }

        float shift = pitchAngleToTranslation(w, displayPitch-degrees);

        // TODO: Intrusion detection and evasion. That is, don't draw
        // where the compass has intruded.

        painter.translate(0, shift);
        QPointF start(-linewidth*w, 0);
        QPointF end(linewidth*w, 0);
        painter.drawLine(start, end);

        if (isMajor && (drawNumbersLeft||drawNumbersRight)) {
            int displayDegrees = degrees;
            if(displayDegrees>90) displayDegrees = 180-displayDegrees;
            else if (displayDegrees<-90) displayDegrees = -180 - displayDegrees;
            if (SHOW_ZERO_ON_SCALES || degrees) {
                QString s_number;
                if (isnan(this->pitch))
                    s_number.sprintf("-");
                else
                    s_number.sprintf("%d", displayDegrees);
                if (drawNumbersLeft)  drawTextRightCenter(painter, s_number, mediumTextSize, -PITCH_SCALE_MAJORWIDTH * w-10, 0);
                if (drawNumbersRight) drawTextLeftCenter(painter, s_number, mediumTextSize, PITCH_SCALE_MAJORWIDTH * w+10, 0);
            }
        }

        painter.setTransform(savedTransform);
    }
}

void PrimaryFlightDisplay::drawRollScale(
        QPainter& painter,
        QRectF area,
        bool drawTicks,
        bool drawNumbers) {

    qreal w = area.width();
    if (w<area.height()) w = area.height();

    QPen pen;
    pen.setWidthF(lineWidth);
    pen.setColor(Qt::white);
    painter.setPen(pen);

    // We should really do these transforms but they are assumed done by caller:
    // painter.resetTransform();
    // painter.translate(area.center());
    // painter.rotate(roll);

    qreal _size = w * ROLL_SCALE_RADIUS*2;
    QRectF arcArea(-_size/2, - _size/2, _size, _size);
    painter.drawArc(arcArea, (90-ROLL_SCALE_RANGE)*16, ROLL_SCALE_RANGE*2*16);
    if (drawTicks) {
        int length = sizeof(tickValues)/sizeof(int);
        qreal previousRotation = 0;
        for (int i=0; i<length*2+1; i++) {
            int degrees = (i==length) ? 0 : (i>length) ?-tickValues[i-length-1] : tickValues[i];
            painter.rotate(degrees - previousRotation);
            previousRotation = degrees;

            QPointF start(0, -_size/2);
            QPointF end(0, -(1.0+ROLL_SCALE_TICKMARKLENGTH)*_size/2);

            painter.drawLine(start, end);

            QString s_number;
            if (SHOW_ZERO_ON_SCALES || degrees)
                s_number.sprintf("%d", abs(degrees));

            if (drawNumbers) {
                drawTextCenterBottom(painter, s_number, mediumTextSize, 0, -(ROLL_SCALE_RADIUS+ROLL_SCALE_TICKMARKLENGTH*1.7)*w);
            }
        }
    }
}

void PrimaryFlightDisplay::drawAIAttitudeScales(
        QPainter& painter,
        QRectF area,
        float intrusion
        ) {
    float displayRoll = this->roll;
    if (isnan(displayRoll))
        displayRoll = 0;
    // To save computations, we do these transformations once for both scales:
    painter.resetTransform();
    painter.translate(area.center());
    painter.rotate(-displayRoll);
    QTransform saved = painter.transform();

    drawRollScale(painter, area, true, true);
    painter.setTransform(saved);
    drawPitchScale(painter, area, intrusion, true, true);
}

void PrimaryFlightDisplay::drawAICompassDisk(QPainter& painter, QRectF area, float halfspan) {
    float displayHeading = this->heading;
    if (isnan(displayHeading))
        displayHeading = 0;

    float start = displayHeading - halfspan;
    float end = displayHeading + halfspan;

    int firstTick = ceil(start / COMPASS_DISK_RESOLUTION) * COMPASS_DISK_RESOLUTION;
    int lastTick = floor(end / COMPASS_DISK_RESOLUTION) * COMPASS_DISK_RESOLUTION;

    float radius = area.width()/2;
    float innerRadius = radius * 0.96;
    painter.resetTransform();
    painter.setBrush(instrumentBackground);
    painter.setPen(instrumentEdgePen);
    painter.drawEllipse(area);
    painter.setBrush(Qt::NoBrush);

    QPen scalePen(Qt::black);
    scalePen.setWidthF(fineLineWidth);

    for (int tickYaw = firstTick; tickYaw <= lastTick; tickYaw += COMPASS_DISK_RESOLUTION) {
        int displayTick = tickYaw;
        if (displayTick < 0) displayTick+=360;
        else if (displayTick>=360) displayTick-=360;

        // yaw is in center.
        float off = tickYaw - displayHeading;
        // wrap that to [-180..180]
        if (off<=-180) off+= 360; else if (off>180) off -= 360;

        painter.translate(area.center());
        painter.rotate(off);
        bool drewArrow = false;
        bool isMajor = displayTick % COMPASS_DISK_MAJORTICK == 0;

        // If heading unknown, still draw marks but no numbers.
        if (!isnan(this->heading) &&
                (displayTick==30 || displayTick==60 ||
                displayTick==120 || displayTick==150 ||
                displayTick==210 || displayTick==240 ||
                displayTick==300 || displayTick==330)
        ) {
            // draw a number
            QString s_number;
            s_number.sprintf("%d", displayTick/10);
            painter.setPen(scalePen);
            drawTextCenter(painter, s_number, smallTextSize, 0, -innerRadius*0.75);
        } else {
            if (displayTick % COMPASS_DISK_ARROWTICK == 0) {
                if (displayTick!=0) {
                    QPainterPath markerPath(QPointF(0, -innerRadius*(1-COMPASS_DISK_MARKERHEIGHT/2)));
                    markerPath.lineTo(innerRadius*COMPASS_DISK_MARKERWIDTH/4, -innerRadius);
                    markerPath.lineTo(-innerRadius*COMPASS_DISK_MARKERWIDTH/4, -innerRadius);
                    markerPath.closeSubpath();
                    painter.setPen(scalePen);
                    painter.setBrush(Qt::SolidPattern);
                    painter.drawPath(markerPath);
                    painter.setBrush(Qt::NoBrush);
                    drewArrow = true;
                }
                // If heading unknown, still draw marks but no N S E W.
                if (!isnan(this->heading) && displayTick%90 == 0) {
                    // Also draw a label
                    QString name = compassWindNames[displayTick / 45];
                    painter.setPen(scalePen);
                    drawTextCenter(painter, name, mediumTextSize, 0, -innerRadius*0.75);
                }
            }
        }
        // draw the scale lines. If an arrow was drawn, stay off from it.

        QPointF p_start = drewArrow ? QPoint(0, -innerRadius*0.94) : QPoint(0, -innerRadius);
        QPoint p_end = isMajor ? QPoint(0, -innerRadius*0.86) : QPoint(0, -innerRadius*0.90);

        painter.setPen(scalePen);
        painter.drawLine(p_start, p_end);
        painter.resetTransform();
    }

    painter.setPen(scalePen);
    painter.translate(area.center());
    QPainterPath markerPath(QPointF(0, -radius-2));
    markerPath.lineTo(radius*COMPASS_DISK_MARKERWIDTH/2,  -radius-radius*COMPASS_DISK_MARKERHEIGHT-2);
    markerPath.lineTo(-radius*COMPASS_DISK_MARKERWIDTH/2, -radius-radius*COMPASS_DISK_MARKERHEIGHT-2);
    markerPath.closeSubpath();
    painter.drawPath(markerPath);

    qreal digitalCompassYCenter = -radius*0.52;
    qreal digitalCompassHeight = radius*0.28;

    QPointF digitalCompassBottom(0, digitalCompassYCenter+digitalCompassHeight);
    QPointF  digitalCompassAbsoluteBottom = painter.transform().map(digitalCompassBottom);

    qreal digitalCompassUpshift = digitalCompassAbsoluteBottom.y()>height() ? digitalCompassAbsoluteBottom.y()-height() : 0;

    QRectF digitalCompassRect(-radius/3, -radius*0.52-digitalCompassUpshift, radius*2/3, radius*0.28);
    painter.setPen(instrumentEdgePen);
    painter.drawRoundedRect(digitalCompassRect, instrumentEdgePen.widthF()*2/3, instrumentEdgePen.widthF()*2/3);

    QString s_digitalCompass;

    if (isnan(this->heading))
        s_digitalCompass.sprintf("---");
    else {
    /* final safeguard for really stupid systems */
        int digitalCompassValue = static_cast<int>(qRound((double)heading)) % 360;
        s_digitalCompass.sprintf("%03d", digitalCompassValue);
    }

    QPen pen;
    pen.setWidthF(lineWidth);
    pen.setColor(Qt::white);
    painter.setPen(pen);

    drawTextCenter(painter, s_digitalCompass, largeTextSize, 0, -radius*0.38-digitalCompassUpshift);

    // The CDI
    if (shouldDisplayNavigationData() && !isnan(navigationTargetBearing) && !isinf(navigationCrosstrackError)) {
        painter.resetTransform();
        painter.translate(area.center());
        // TODO : Sign might be wrong?
        // TODO : The case where error exceeds max. Truncate to max. and make that visible somehow.
        bool errorBeyondRadius = false;
        if (abs(navigationCrosstrackError) > CROSSTRACK_MAX) {
            errorBeyondRadius = true;
            navigationCrosstrackError = navigationCrosstrackError>0 ? CROSSTRACK_MAX : -CROSSTRACK_MAX;
        }

        float r = radius * CROSSTRACK_RADIUS;
        float x = navigationCrosstrackError / CROSSTRACK_MAX * r;
        float y = qSqrt(r*r - x*x); // the positive y, there is also a negative.

        float sillyHeading = 0;
        float angle = sillyHeading - navigationTargetBearing; // TODO: sign.
        painter.rotate(-angle);

        QPen pen;
        pen.setWidthF(lineWidth);
        pen.setColor(Qt::black);
        if(errorBeyondRadius) {
            pen.setStyle(Qt::DotLine);
        }
        painter.setPen(pen);

        painter.drawLine(QPointF(x, y), QPointF(x, -y));
    }
}

void PrimaryFlightDisplay::drawAltimeter(
        QPainter& painter,
        QRectF area
    ) {

    float primaryAltitude = altitudeWGS84;
    float secondaryAltitude = std::numeric_limits<double>::quiet_NaN();

    painter.resetTransform();
    fillInstrumentBackground(painter, area);

    QPen pen;
    pen.setWidthF(lineWidth);

    pen.setColor(Qt::white);
    painter.setPen(pen);

    float h = area.height();
    float w = area.width();
    float secondaryAltitudeBoxHeight = mediumTextSize * 2;
    // The height where we being with new tickmarks.
    float effectiveHalfHeight = h*0.45;

    // not yet implemented: Display of secondary altitude.
    if (!isnan(secondaryAltitude)) {
        effectiveHalfHeight -= secondaryAltitudeBoxHeight;
    }

    float markerHalfHeight = mediumTextSize;
    float leftEdge = instrumentEdgePen.widthF()*2;
    float rightEdge = w-leftEdge;
    float tickmarkLeft = leftEdge;
    float tickmarkRightMajor = tickmarkLeft+TAPE_GAUGES_TICKWIDTH_MAJOR*w;
    float tickmarkRightMinor = tickmarkLeft+TAPE_GAUGES_TICKWIDTH_MINOR*w;
    float numbersLeft = 0.42*w;
    float markerTip = (tickmarkLeft*2+tickmarkRightMajor)/3;
	float markerOffset = 0.2* markerHalfHeight;
	float scaleCenterAltitude = isnan(primaryAltitude) ? 0 : primaryAltitude;
	
    // altitude scale
    float start = scaleCenterAltitude - ALTIMETER_LINEAR_SPAN/2;
    float end = scaleCenterAltitude + ALTIMETER_LINEAR_SPAN/2;
    int firstTick = ceil(start / ALTIMETER_LINEAR_RESOLUTION) * ALTIMETER_LINEAR_RESOLUTION;
    int lastTick = floor(end / ALTIMETER_LINEAR_RESOLUTION) * ALTIMETER_LINEAR_RESOLUTION;
    for (int tickAlt = firstTick; tickAlt <= lastTick; tickAlt += ALTIMETER_LINEAR_RESOLUTION) {
        float y = (tickAlt-scaleCenterAltitude)*effectiveHalfHeight/(ALTIMETER_LINEAR_SPAN/2);
        bool isMajor = tickAlt % ALTIMETER_LINEAR_MAJOR_RESOLUTION == 0;

        painter.resetTransform();
        painter.translate(area.left(), area.center().y() - y);
        pen.setColor(tickAlt<0 ? redColor : Qt::white);
        painter.setPen(pen);
        if (isMajor) {
            painter.drawLine(tickmarkLeft, 0, tickmarkRightMajor, 0);
            QString s_alt;
            s_alt.sprintf("%d", abs(tickAlt));
            drawTextLeftCenter(painter, s_alt, mediumTextSize, numbersLeft, 0);
        } else {
            painter.drawLine(tickmarkLeft, 0, tickmarkRightMinor, 0);
        }
    }

    QPainterPath primaryMarkerPath(QPoint(markerTip, 0));
	primaryMarkerPath.lineTo(markerTip + markerHalfHeight, markerHalfHeight);
	primaryMarkerPath.lineTo(rightEdge, markerHalfHeight);
	primaryMarkerPath.lineTo(rightEdge, -markerHalfHeight);
	primaryMarkerPath.lineTo(markerTip + markerHalfHeight, -markerHalfHeight);
	primaryMarkerPath.closeSubpath();

	QPainterPath secondaryMarkerPath(QPoint(markerTip + markerHalfHeight, markerHalfHeight + markerOffset));
	if (!isnan(climbRate)) {
		secondaryMarkerPath.lineTo(markerTip + markerHalfHeight, 2 * markerHalfHeight + markerOffset);
		secondaryMarkerPath.lineTo(rightEdge, 2 * markerHalfHeight + markerOffset);
		secondaryMarkerPath.lineTo(rightEdge, 1 * markerHalfHeight + markerOffset);
		secondaryMarkerPath.closeSubpath();
	}

	painter.resetTransform();
    painter.translate(area.left(), area.center().y());

    pen.setWidthF(lineWidth);
    pen.setColor(Qt::white);
    painter.setPen(pen);

    painter.setBrush(Qt::SolidPattern);
    painter.drawPath(primaryMarkerPath);
	if (!isnan(climbRate)) painter.drawPath(secondaryMarkerPath);
    painter.setBrush(Qt::NoBrush);

    pen.setColor(Qt::white);
    painter.setPen(pen);
	
    QString s_alt;
    if (isnan(primaryAltitude))
        s_alt.sprintf("---");
    else
        s_alt.sprintf("h:%3.0f", primaryAltitude);

    drawTextRightCenter(painter, s_alt, mediumTextSize, rightEdge - 4 * lineWidth, 0);

    // draw simple in-tape VVI.
    if (!isnan(climbRate)) {
		// Draw label
		QString s_climb;
		s_climb.sprintf("vZ:%2.1f", climbRate);
		drawTextRightCenter(painter, s_climb, smallTextSize, rightEdge - 4 * lineWidth, 1.5*mediumTextSize + markerOffset);
		
		// Draw climb rate indicator as an arrow
		float vvPixHeight = -climbRate/ALTIMETER_VVI_SPAN * effectiveHalfHeight;
        if (abs (vvPixHeight) < markerHalfHeight)
            return; // hidden behind marker.

        float vvSign = vvPixHeight>0 ? 1 : -1; // reverse y sign

		QPointF vvArrowBegin(rightEdge - w*ALTIMETER_VVI_WIDTH / 2, (vvSign ? 2*markerHalfHeight+markerOffset : markerHalfHeight));
        QPointF vvArrowEnd(rightEdge - w*ALTIMETER_VVI_WIDTH/2, vvPixHeight);
        painter.drawLine(vvArrowBegin, vvArrowEnd);

        // Yeah this is a repetition of above code but we are going to trash it all anyway, so no fix.
        float vvArowHeadSize = abs(vvPixHeight - markerHalfHeight*vvSign);
        if (vvArowHeadSize > w*ALTIMETER_VVI_WIDTH/3) vvArowHeadSize = w*ALTIMETER_VVI_WIDTH/3;

        float xcenter = rightEdge-w*ALTIMETER_VVI_WIDTH/2;

        QPointF vvArrowHead(xcenter+vvArowHeadSize, vvPixHeight - vvSign *vvArowHeadSize);
        painter.drawLine(vvArrowHead, vvArrowEnd);

        vvArrowHead = QPointF(xcenter-vvArowHeadSize, vvPixHeight - vvSign * vvArowHeadSize);
        painter.drawLine(vvArrowHead, vvArrowEnd);
    }

    // print secondary altitude
    if (!isnan(secondaryAltitude)) {
        QRectF saBox(area.x(), area.y()-secondaryAltitudeBoxHeight, w, secondaryAltitudeBoxHeight);
        painter.resetTransform();
        painter.translate(saBox.center());
        QString s_salt;
        s_salt.sprintf("%3.0f", secondaryAltitude);
        drawTextCenter(painter, s_salt, mediumTextSize, 0, 0);
    }
}

void PrimaryFlightDisplay::drawVelocityMeter(
	QPainter& painter,
	QRectF area
	) {

	painter.resetTransform();
	fillInstrumentBackground(painter, area);

	QPen pen;
	pen.setWidthF(lineWidth);

	float h = area.height();
	float w = area.width();
	float effectiveHalfHeight = h*0.45;
	float markerHalfHeight = mediumTextSize;
	float leftEdge = instrumentEdgePen.widthF() * 2;
	float tickmarkRight = w - leftEdge;
	float tickmarkLeftMajor = tickmarkRight - w*TAPE_GAUGES_TICKWIDTH_MAJOR;
	float tickmarkLeftMinor = tickmarkRight - w*TAPE_GAUGES_TICKWIDTH_MINOR;
	float numbersRight = 0.42*w;
	float markerTip = (tickmarkLeftMajor + tickmarkRight * 2) / 3;
	float markerOffset = 0.2 * markerHalfHeight;

	// Select between air and ground speed:
	bool bSpeedIsAirspeed = (isAirplane() && !isnan(airSpeed));
	float primarySpeed = bSpeedIsAirspeed ? airSpeed : groundSpeed;
	float secondarySpeed = !bSpeedIsAirspeed ? airSpeed : groundSpeed;
	float centerScaleSpeed = isnan(primarySpeed) ? 0 : primarySpeed;
	
	float start = centerScaleSpeed - AIRSPEED_LINEAR_SPAN / 2;
	float end = centerScaleSpeed + AIRSPEED_LINEAR_SPAN / 2;

	int firstTick = ceil(start / AIRSPEED_LINEAR_RESOLUTION) * AIRSPEED_LINEAR_RESOLUTION;
	int lastTick = floor(end / AIRSPEED_LINEAR_RESOLUTION) * AIRSPEED_LINEAR_RESOLUTION;
	for (int tickSpeed = firstTick; tickSpeed <= lastTick; tickSpeed += AIRSPEED_LINEAR_RESOLUTION) {
		pen.setColor(tickSpeed < 0 ? redColor : Qt::white);
		painter.setPen(pen);

		float y = (tickSpeed - centerScaleSpeed)*effectiveHalfHeight / (AIRSPEED_LINEAR_SPAN / 2);
		bool hasText = tickSpeed % AIRSPEED_LINEAR_MAJOR_RESOLUTION == 0;
		painter.resetTransform();

		painter.translate(area.left(), area.center().y() - y);

		if (hasText) {
			painter.drawLine(tickmarkLeftMajor, 0, tickmarkRight, 0);
			QString s_speed;
			s_speed.sprintf("%d", abs(tickSpeed));
			drawTextRightCenter(painter, s_speed, mediumTextSize, numbersRight, 0);
		}
		else {
			painter.drawLine(tickmarkLeftMinor, 0, tickmarkRight, 0);
		}
	}

	//Paint the label background
	QPainterPath primaryMarkerPath(QPoint(markerTip, 0));
	primaryMarkerPath.lineTo(markerTip - markerHalfHeight, markerHalfHeight);
	primaryMarkerPath.lineTo(leftEdge, markerHalfHeight);
	primaryMarkerPath.lineTo(leftEdge, -markerHalfHeight);
	primaryMarkerPath.lineTo(markerTip - markerHalfHeight, -markerHalfHeight);
	primaryMarkerPath.closeSubpath();

	QPainterPath secondaryMarkerPath(QPoint(markerTip - markerHalfHeight, 1 * markerHalfHeight + markerOffset));
	if (!isnan(secondarySpeed)) {
		secondaryMarkerPath.lineTo(markerTip - markerHalfHeight, 2 * markerHalfHeight + markerOffset);
		secondaryMarkerPath.lineTo(leftEdge, 2 * markerHalfHeight + markerOffset);
		secondaryMarkerPath.lineTo(leftEdge, 1 * markerHalfHeight + markerOffset);
		secondaryMarkerPath.closeSubpath();
	}
	
	painter.resetTransform();
	painter.translate(area.left(), area.center().y());

	pen.setWidthF(lineWidth);
	pen.setColor(Qt::white);
	painter.setPen(pen);

	painter.setBrush(Qt::SolidPattern);
	painter.drawPath(primaryMarkerPath);
	if (!isnan(secondarySpeed)) painter.drawPath(secondaryMarkerPath);
	painter.setBrush(Qt::NoBrush);

	// Draw primary speed
	pen.setColor(Qt::white);
	painter.setPen(pen);
	QString s_alt;
	if (isnan(primarySpeed))
		s_alt.sprintf("---");
	else
		s_alt.sprintf("%s:%3.1f", (bSpeedIsAirspeed ? "AS" : "GS"), primarySpeed);
	drawTextLeftCenter(painter, s_alt, mediumTextSize, 4 * lineWidth, 0);

	// Draw secondary speed
	if (!isnan(secondarySpeed)) {
		pen.setColor(Qt::white);
		painter.setPen(pen);
		s_alt.sprintf("%s:%3.1f", (!bSpeedIsAirspeed ? "AS" : "GS"), secondarySpeed);
		drawTextLeftCenter(painter, s_alt, smallTextSize, 4 * lineWidth, 1.5 * markerHalfHeight + markerOffset);
	}
}

static const int TOP = (1<<0);
static const int BOTTOM = (1<<1);
static const int LEFT = (1<<2);
static const int RIGHT = (1<<3);

static const int TOP_HALF = (1<<4);
static const int BOTTOM_HALF = (1<<5);
static const int LEFT_HALF = (1<<6);
static const int RIGHT_HALF = (1<<7);

void applyMargin(QRectF& area, float margin, int where) {
    if (margin < 0.01) return;

    QRectF save(area);
    qreal consumed;

    if (where & LEFT) {
        area.setX(save.x() + (consumed = margin));
    } else if (where & LEFT_HALF) {
        area.setX(save.x() + (consumed = margin/2));
    } else {
        consumed = 0;
    }

    if (where & RIGHT) {
        area.setWidth(save.width()-consumed-margin);
    } else if (where & RIGHT_HALF) {
        area.setWidth(save.width()-consumed-margin/2);
    } else {
        area.setWidth(save.width()-consumed);
    }

    if (where & TOP) {
        area.setY(save.y() + (consumed = margin));
    } else if (where & TOP_HALF) {
        area.setY(save.y() + (consumed = margin/2));
    } else {
        consumed = 0;
    }

    if (where & BOTTOM) {
        area.setHeight(save.height()-consumed-margin);
    } else if (where & BOTTOM_HALF) {
        area.setHeight(save.height()-consumed-margin/2);
    } else {
        area.setHeight(save.height()-consumed);
    }
}

void setMarginsForInlineLayout(qreal margin, QRectF& panel1, QRectF& panel2, QRectF& panel3, QRectF& panel4) {
    applyMargin(panel1, margin, BOTTOM|LEFT|RIGHT_HALF);
    applyMargin(panel2, margin, BOTTOM|LEFT_HALF|RIGHT_HALF);
    applyMargin(panel3, margin, BOTTOM|LEFT_HALF|RIGHT_HALF);
    applyMargin(panel4, margin, BOTTOM|LEFT_HALF|RIGHT);
}

void setMarginsForCornerLayout(qreal margin, QRectF& panel1, QRectF& panel2, QRectF& panel3, QRectF& panel4) {
    applyMargin(panel1, margin, BOTTOM|LEFT|RIGHT_HALF);
    applyMargin(panel2, margin, BOTTOM|LEFT_HALF|RIGHT_HALF);
    applyMargin(panel3, margin, BOTTOM|LEFT_HALF|RIGHT_HALF);
    applyMargin(panel4, margin, BOTTOM|LEFT_HALF|RIGHT);
}

inline qreal tapesGaugeWidthFor(qreal containerWidth, qreal preferredAIWidth) {
    qreal result = (containerWidth - preferredAIWidth) / 2.0f;
    qreal minimum = containerWidth / 5.5f;
    if (result < minimum) result = minimum;
    return result;
}

void PrimaryFlightDisplay::doPaint() {
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    qreal margin = height()/100.0f;

    // The AI centers on this area.
    QRectF AIMainArea;
    // The AI paints on this area. It should contain the AIMainArea.
    QRectF AIPaintArea;

    QRectF compassArea;
    QRectF altimeterArea;
    QRectF velocityMeterArea;
    QRectF sensorsStatsArea;
    QRectF linkStatsArea;
    QRectF sysStatsArea;
    QRectF missionStatsArea;

    painter.fillRect(rect(), Qt::black);
    qreal tapeGaugeWidth;

    qreal compassHalfSpan = 180;
    float compassAIIntrusion = 0;

    switch(layout) {
    case COMPASS_INTEGRATED: {
        tapeGaugeWidth = tapesGaugeWidthFor(width(), width());
        qreal aiheight = height();
        qreal aiwidth = width()-tapeGaugeWidth*2;
        if (aiheight > aiwidth) aiheight = aiwidth;

        AIMainArea = QRectF(
                    tapeGaugeWidth,
                    0,
                    aiwidth,
                    aiheight);

        AIPaintArea = QRectF(
                    0,
                    0,
                    width(),
                    height());

        // Tape gauges get so much width that the AI area not covered by them is perfectly square.
        velocityMeterArea = QRectF (0, 0, tapeGaugeWidth, aiheight);
        altimeterArea = QRectF(AIMainArea.right(), 0, tapeGaugeWidth, aiheight);

        if (style == NO_OVERLAYS) {
            applyMargin(AIMainArea, margin, TOP|BOTTOM);
            applyMargin(altimeterArea, margin, TOP|BOTTOM|RIGHT);
            applyMargin(velocityMeterArea, margin, TOP|BOTTOM|LEFT);
            setMarginsForInlineLayout(margin, sensorsStatsArea, linkStatsArea, sysStatsArea, missionStatsArea);
        }

        qreal compassRelativeWidth = 0.75;
        qreal compassBottomMargin = 0.78;

        qreal compassSize = compassRelativeWidth  * AIMainArea.width();  // Diameter is this times the width.

        qreal compassCenterY;
        compassCenterY = AIMainArea.bottom() + compassSize / 4;

        if (height() - compassCenterY > AIMainArea.width()/2*compassBottomMargin)
            compassCenterY = height()-AIMainArea.width()/2*compassBottomMargin;

        // TODO: This is bad style...
        compassCenterY = (compassCenterY * 2 + AIMainArea.bottom() + compassSize / 4) / 3;

        compassArea = QRectF(AIMainArea.x()+(1-compassRelativeWidth)/2*AIMainArea.width(),
                             compassCenterY-compassSize/2,
                             compassSize,
                             compassSize);

        if (height()-compassCenterY < compassSize/2) {
            compassHalfSpan = acos((compassCenterY-height())*2/compassSize) * 180/M_PI + COMPASS_DISK_RESOLUTION;
            if (compassHalfSpan > 180) compassHalfSpan = 180;
        }

        compassAIIntrusion = compassSize/2 + AIMainArea.bottom() - compassCenterY;
        if (compassAIIntrusion<0) compassAIIntrusion = 0;

        break;
    }
    case COMPASS_SEPARATED: {
        // A layout for containers higher than their width.
        tapeGaugeWidth = tapesGaugeWidthFor(width(), width());

        qreal aiheight = width() - tapeGaugeWidth*2;
        qreal panelsHeight = 0;

        AIMainArea = QRectF(
                    tapeGaugeWidth,
                    0,
                    width()-tapeGaugeWidth*2,
                    aiheight);

        AIPaintArea = style == OVERLAY_HSI ?
                    QRectF(
                    0,
                    0,
                    width(),
                    height() - panelsHeight) : AIMainArea;

        velocityMeterArea = QRectF (0, 0, tapeGaugeWidth, aiheight);
        altimeterArea = QRectF(AIMainArea.right(), 0, tapeGaugeWidth, aiheight);

        QPoint compassCenter = QPoint(width()/2, AIMainArea.bottom()+width()/2);
        qreal compassDiam = width() * 0.8;
        compassArea = QRectF(compassCenter.x()-compassDiam/2, compassCenter.y()-compassDiam/2, compassDiam, compassDiam);
        break;
    }
    }

    bool hadClip = painter.hasClipping();

    painter.setClipping(true);
    painter.setClipRect(AIPaintArea);

    drawAIGlobalFeatures(painter, AIMainArea, AIPaintArea);
    drawAIAttitudeScales(painter, AIMainArea, compassAIIntrusion);
    drawAIAirframeFixedFeatures(painter, AIMainArea);

    drawAICompassDisk(painter, compassArea, compassHalfSpan);

    painter.setClipping(hadClip);

    drawAltimeter(painter, altimeterArea);

    drawVelocityMeter(painter, velocityMeterArea);

    painter.end();
}

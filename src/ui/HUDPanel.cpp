#include "HUDPanel.h"
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
static const int ALTIMETER_LINEAR_RESOLUTION = 10;
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
double HUDPanel_round(double value, int digits=0)
{
    return floor(value * pow(10.0, digits) + 0.5) / pow(10.0, digits);
}

qreal HUDPanel_constrain(qreal value, qreal min, qreal max) {
    if (value<min) value=min;
    else if(value>max) value=max;
    return value;
}

const int HUDPanel::tickValues[] = {10, 20, 30, 45, 60};
const QString HUDPanel::compassWindNames[] = {
    QString("N"),
    QString("NE"),
    QString("E"),
    QString("SE"),
    QString("S"),
    QString("SW"),
    QString("W"),
    QString("NW")
};

HUDPanel::HUDPanel(QWidget *parent) :
    QWidget(parent),

    _valuesChanged(false),
    _valuesLastPainted(QGC::groundTimeMilliseconds()),

    uas(NULL),

    roll(0),
    pitch(0),
    heading(0),

    altitudeAMSL(std::numeric_limits<double>::quiet_NaN()),
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

HUDPanel::~HUDPanel()
{
    refreshTimer->stop();
}

QSize HUDPanel::sizeHint() const
{
    return QSize(width(), (int)(width() * 3.0f / 4.0f));
}


void HUDPanel::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display) events
    QWidget::showEvent(event);
    refreshTimer->start(updateInterval);
    emit visibilityChanged(true);
}

void HUDPanel::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display) events
    refreshTimer->stop();
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void HUDPanel::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);

    qreal size = e->size().width();

    lineWidth = HUDPanel_constrain(size*LINEWIDTH, 1, 6);
    fineLineWidth = HUDPanel_constrain(size*LINEWIDTH*2/3, 1, 2);

    instrumentEdgePen.setWidthF(fineLineWidth);

    smallTextSize = size * SMALL_TEXT_SIZE;
    mediumTextSize = size * MEDIUM_TEXT_SIZE;
    largeTextSize = size * LARGE_TEXT_SIZE;
}

void HUDPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    doPaint();
}

void HUDPanel::checkUpdate()
{
    if (uas && (_valuesChanged || (QGC::groundTimeMilliseconds() - _valuesLastPainted) > 260)) {
        update();
        _valuesChanged = false;
        _valuesLastPainted = QGC::groundTimeMilliseconds();
    }
}

void HUDPanel::forgetUAS(UASInterface* uas)
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
void HUDPanel::setActiveUAS(UASInterface* uas)
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

void HUDPanel::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp)
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

void HUDPanel::updateAttitude(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp)
{
    Q_UNUSED(component);
    this->updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void HUDPanel::updateSpeed(UASInterface* uas, double _groundSpeed, double _airSpeed, quint64 timestamp)
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

void HUDPanel::updateAltitude(UASInterface* uas, double _altitudeAMSL, double _altitudeWGS84, double _altitudeRelative, double _climbRate, quint64 timestamp) {
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

void HUDPanel::updateNavigationControllerErrors(UASInterface* uas, double altitudeError, double speedError, double xtrackError) {
    Q_UNUSED(uas);
    this->navigationAltitudeError = altitudeError;
    this->navigationSpeedError = speedError;
    this->navigationCrosstrackError = xtrackError;
}


/*
 * Private and such
 */

// TODO: Move to UAS. Real working implementation.
bool HUDPanel::isAirplane() {
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
bool HUDPanel::shouldDisplayNavigationData() {
    return true;
}

void HUDPanel::drawTextCenter (
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

void HUDPanel::drawTextLeftCenter (
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

void HUDPanel::drawTextRightCenter (
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

void HUDPanel::drawTextCenterTop (
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

void HUDPanel::drawTextCenterBottom (
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

void HUDPanel::drawInstrumentBackground(QPainter& painter, QRectF edge) {
    painter.setPen(instrumentEdgePen);
    painter.drawRect(edge);
}

void HUDPanel::fillInstrumentBackground(QPainter& painter, QRectF edge) {
    painter.setPen(instrumentEdgePen);
    painter.setBrush(instrumentBackground);
    painter.drawRect(edge);
    painter.setBrush(Qt::NoBrush);
}

void HUDPanel::fillInstrumentOpagueBackground(QPainter& painter, QRectF edge) {
    painter.setPen(instrumentEdgePen);
    painter.setBrush(instrumentOpagueBackground);
    painter.drawRect(edge);
    painter.setBrush(Qt::NoBrush);
}


void HUDPanel::drawAIAirframeFixedFeatures(QPainter& painter, QRectF area) {
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

void HUDPanel::drawAIGlobalFeatures(
        QPainter& painter,
        QRectF mainArea,
        QRectF paintArea) {

    float displayRoll = this->roll;
    if (isnan(displayRoll))
        displayRoll = 0;

    painter.resetTransform();
    painter.translate(mainArea.center());

}

void HUDPanel::drawPitchScale(
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

        painter.setTransform(savedTransform);
    }
}

void HUDPanel::drawRollScale(
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

void HUDPanel::drawAIAttitudeScales(
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

void HUDPanel::drawAICompassDisk(QPainter& painter, QRectF area, float halfspan) {
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

void HUDPanel::drawAltimeter(
        QPainter& painter,
        QRectF area
    ) {

    float primaryAltitude = altitudeWGS84;
    float secondaryAltitude = 0;

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

    float markerHalfHeight = mediumTextSize*0.8;
    float leftEdge = instrumentEdgePen.widthF()*2;
    float rightEdge = w-leftEdge;
    float tickmarkLeft = leftEdge;
    float tickmarkRightMajor = tickmarkLeft+TAPE_GAUGES_TICKWIDTH_MAJOR*w;
    float tickmarkRightMinor = tickmarkLeft+TAPE_GAUGES_TICKWIDTH_MINOR*w;
    float numbersLeft = 0.42*w;
    float markerTip = (tickmarkLeft*2+tickmarkRightMajor)/3;
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

    QPainterPath markerPath(QPoint(markerTip, 0));
    markerPath.lineTo(markerTip+markerHalfHeight, markerHalfHeight);
    markerPath.lineTo(rightEdge, markerHalfHeight);
    markerPath.lineTo(rightEdge, -markerHalfHeight);
    markerPath.lineTo(markerTip+markerHalfHeight, -markerHalfHeight);
    markerPath.closeSubpath();

    painter.resetTransform();
    painter.translate(area.left(), area.center().y());

    pen.setWidthF(lineWidth);
    pen.setColor(Qt::white);
    painter.setPen(pen);

    painter.setBrush(Qt::SolidPattern);
    painter.drawPath(markerPath);
    painter.setBrush(Qt::NoBrush);

    pen.setColor(Qt::white);
    painter.setPen(pen);

    QString s_alt;
    if (isnan(primaryAltitude))
        s_alt.sprintf("---");
    else
        s_alt.sprintf("%3.0f", primaryAltitude);

    float xCenter = (markerTip+rightEdge)/2;
    drawTextCenter(painter, s_alt, mediumTextSize, xCenter, 0);

    // draw simple in-tape VVI.
    if (!isnan(climbRate)) {
        float vvPixHeight = -climbRate/ALTIMETER_VVI_SPAN * effectiveHalfHeight;
        if (abs (vvPixHeight) < markerHalfHeight)
            return; // hidden behind marker.

        float vvSign = vvPixHeight>0 ? 1 : -1; // reverse y sign

        QPointF vvArrowBegin(rightEdge - w*ALTIMETER_VVI_WIDTH/2, markerHalfHeight*vvSign);
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

void HUDPanel::drawVelocityMeter(
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
    float leftEdge = instrumentEdgePen.widthF()*2;
    float tickmarkRight = w-leftEdge;
    float tickmarkLeftMajor = tickmarkRight-w*TAPE_GAUGES_TICKWIDTH_MAJOR;
    float tickmarkLeftMinor = tickmarkRight-w*TAPE_GAUGES_TICKWIDTH_MINOR;
    float numbersRight = 0.42*w;
    float markerTip = (tickmarkLeftMajor+tickmarkRight*2)/3;

    // Select between air and ground speed:
    float speed = (isAirplane() && !isnan(airSpeed)) ? airSpeed : groundSpeed;
    float centerScaleSpeed = isnan(speed) ? 0 : speed;

    float start = centerScaleSpeed - AIRSPEED_LINEAR_SPAN/2;
    float end = centerScaleSpeed + AIRSPEED_LINEAR_SPAN/2;

    int firstTick = ceil(start / AIRSPEED_LINEAR_RESOLUTION) * AIRSPEED_LINEAR_RESOLUTION;
    int lastTick = floor(end / AIRSPEED_LINEAR_RESOLUTION) * AIRSPEED_LINEAR_RESOLUTION;
    for (int tickSpeed = firstTick; tickSpeed <= lastTick; tickSpeed += AIRSPEED_LINEAR_RESOLUTION) {
        pen.setColor(tickSpeed<0 ? redColor : Qt::white);
        painter.setPen(pen);

        float y = (tickSpeed-centerScaleSpeed)*effectiveHalfHeight/(AIRSPEED_LINEAR_SPAN/2);
        bool hasText = tickSpeed % AIRSPEED_LINEAR_MAJOR_RESOLUTION == 0;
        painter.resetTransform();

        painter.translate(area.left(), area.center().y() - y);

        if (hasText) {
            painter.drawLine(tickmarkLeftMajor, 0, tickmarkRight, 0);
            QString s_speed;
            s_speed.sprintf("%d", abs(tickSpeed));
            drawTextRightCenter(painter, s_speed, mediumTextSize, numbersRight, 0);
        } else {
            painter.drawLine(tickmarkLeftMinor, 0, tickmarkRight, 0);
        }
    }

    QPainterPath markerPath(QPoint(markerTip, 0));
    markerPath.lineTo(markerTip-markerHalfHeight, markerHalfHeight);
    markerPath.lineTo(leftEdge, markerHalfHeight);
    markerPath.lineTo(leftEdge, -markerHalfHeight);
    markerPath.lineTo(markerTip-markerHalfHeight, -markerHalfHeight);
    markerPath.closeSubpath();

    painter.resetTransform();
    painter.translate(area.left(), area.center().y());

    pen.setWidthF(lineWidth);
    pen.setColor(Qt::white);
    painter.setPen(pen);

    painter.setBrush(Qt::SolidPattern);
    painter.drawPath(markerPath);
    painter.setBrush(Qt::NoBrush);

    pen.setColor(Qt::white);
    painter.setPen(pen);
    QString s_alt;
    if (isnan(speed))
        s_alt.sprintf("---");
    else
        s_alt.sprintf("%3.1f", speed);
    float xCenter = (markerTip+leftEdge)/2;
    drawTextCenter(painter, s_alt, mediumTextSize, xCenter, 0);
}


void HUDPanel::doPaint() {
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


    painter.end();
}

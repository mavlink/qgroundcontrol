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


HUDPanel::HUDPanel(QWidget *parent) :
    QWidget(parent),

    _valuesChanged(false),
    _valuesLastPainted(QGC::groundTimeMilliseconds()),

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
    setMinimumSize(320, 240);
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

    painter.fillRect(rect(), Qt::red);

    painter.end();
}

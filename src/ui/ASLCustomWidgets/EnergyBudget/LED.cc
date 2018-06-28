#include <math.h>

#include <QPainter>
#include <QGradient>
#include <QPaintDevice>
#include <QTimer>

#include "LED.h"

LED::
LED(QWidget* parent) :
	QWidget(parent),
	diameter_(5),
	color_(QColor("red")),
	alignment_(Qt::AlignCenter),
	initialState_(true),
	state_(true),
	flashRate_(200),
	flashing_(false)
{
	timer_ = new QTimer(this);
	connect(timer_, SIGNAL(timeout()), this, SLOT(toggleState()));

    setDiameter(diameter_);
}

LED::
~LED()
{
}


double LED::
diameter() const
{
	return diameter_;
}

void LED::
setDiameter(double diameter)
{
	diameter_ = diameter;

	pixX_ = round(double(height())/heightMM());
	pixY_ = round(double(width())/widthMM());

	diamX_ = diameter_*pixX_;
	diamY_ = diameter_*pixY_;

	update();
}


QColor LED::
color() const
{
	return color_;
}

void LED::
setColor(const QColor& color)
{
	color_ = color;
	update();
}

Qt::Alignment LED::
alignment() const
{
	return alignment_;
}

void LED::
setAlignment(Qt::Alignment alignment)
{
	alignment_ = alignment;

	update();
}

void LED::
setFlashRate(int rate)
{
	flashRate_ = rate;

	update();
}

void LED::
setFlashing(bool flashing)
{
	flashing_ = flashing;

	update();
}

void LED::
startFlashing()
{
	setFlashing(true);
}

void LED::
stopFlashing()
{
	setFlashing(false);
}


void LED::
setState(bool state)
{
	state_ = state;
	update();
}

void LED::
toggleState()
{
	state_ = !state_;
	update();
}

int LED::
heightForWidth(int width) const
{
	return width;
}

QSize LED::
sizeHint() const
{
	return QSize(diamX_, diamY_);
}

QSize LED::
minimumSizeHint() const
{
	return QSize(diamX_, diamY_);
}

void LED::
paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

	QPainter p(this);

	QRect geo = geometry();
	int width = geo.width();
	int height = geo.height();

	int x=0, y=0;
	if ( alignment_ & Qt::AlignLeft )
		x = 0;
	else if ( alignment_ & Qt::AlignRight )
		x = width-diamX_;
	else if ( alignment_ & Qt::AlignHCenter )
		x = (width-diamX_)/2;
	else if ( alignment_ & Qt::AlignJustify )
		x = 0;

	if ( alignment_ & Qt::AlignTop )
		y = 0;
	else if ( alignment_ & Qt::AlignBottom )
		y = height-diamY_;
	else if ( alignment_ & Qt::AlignVCenter )
		y = (height-diamY_)/2;

	QRadialGradient g(x+diamX_/2, y+diamY_/2, diamX_*0.4,
		diamX_*0.4, diamY_*0.4);

	g.setColorAt(0, Qt::white);
	if ( state_ )
		g.setColorAt(1, color_);
	else
		g.setColorAt(1, Qt::black);
	QBrush brush(g);

	p.setPen(color_);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setBrush(brush);
	p.drawEllipse(x, y, diamX_-1, diamY_-1);

	if ( flashRate_ > 0 && flashing_ )
		timer_->start(flashRate_);
	else
		timer_->stop();
}

bool LED::
state() const
{
    return state_;
}

bool LED::
isFlashing() const
{
    return flashing_;
}
    
int LED::
flashRate() const
{
    return flashRate_;
}

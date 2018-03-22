#ifndef _LED_H_
#define _LED_H_

#include <QWidget>

class QTimer;

class LED : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(double diameter READ diameter WRITE setDiameter) // mm
	Q_PROPERTY(QColor color READ color WRITE setColor)
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
	Q_PROPERTY(bool state READ state WRITE setState)
	Q_PROPERTY(bool flashing READ isFlashing WRITE setFlashing)
	Q_PROPERTY(int flashRate READ flashRate WRITE setFlashRate)

public:
	explicit LED(QWidget* parent=0);
	~LED();

	double diameter() const;
	void setDiameter(double diameter);

	QColor color() const;
	void setColor(const QColor& color);

	Qt::Alignment alignment() const;
	void setAlignment(Qt::Alignment alignment);

    bool state() const;

    bool isFlashing() const;
    
    int flashRate() const;

public slots:
	void setState(bool state);
	void toggleState();
	void setFlashing(bool flashing);
	void setFlashRate(int rate);
	void startFlashing();
	void stopFlashing();

public:
	int heightForWidth(int width) const;
	QSize sizeHint() const;
	QSize minimumSizeHint() const;

protected:
	void paintEvent(QPaintEvent* event);

private:
	double diameter_;
	QColor color_;
	Qt::Alignment alignment_;
	bool initialState_;
	bool state_;
	int flashRate_;
	bool flashing_;

	//
	// Pixels per mm for x and y...
	//
	int pixX_, pixY_;

	//
	// Scaled values for x and y diameter.
	//
	int diamX_, diamY_;

	QRadialGradient gradient_;
	QTimer* timer_;
};

#endif

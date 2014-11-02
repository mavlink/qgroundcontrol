#ifndef HLED_H
#define HLED_H

#include <QWidget>

class QColor;

class HLed : public QWidget
{
    Q_OBJECT
public:
    HLed(QWidget *parent = 0);
    ~HLed();

    QColor color() const;
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

public slots:
    void setColor(const QColor &color);
    void toggle();
    void turnOn(bool on=true);
    void turnOff(bool off=true);

protected:
    void paintEvent(QPaintEvent *);
    int ledWidth() const;

private:
    struct Private;
    Private * const m_d;
};

#endif // HLED_H

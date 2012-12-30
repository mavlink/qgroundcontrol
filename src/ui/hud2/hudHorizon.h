#ifndef HUDHORIZON_H
#define HUDHORIZON_H

#include <QWidget>

class HudHorizon : public QWidget
{
    Q_OBJECT
public:
    explicit HudHorizon(QWidget *parent = 0);

signals:
    
public slots:

private:
    float *roll;
    float *pitch;
};

#endif // HUDHORIZON_H

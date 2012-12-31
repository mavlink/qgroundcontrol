#ifndef HUDPAINTER_NEW_H
#define HUDPAINTER_NEW_H

#include <QWidget>
#include <QBrush>
#include <QFont>
#include <QPen>

#include "hudYawIndicator.h"
#include "hudHorizon.h"
#include "hudData.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

class hudPainter : public QWidget
{
    Q_OBJECT
public:
    explicit hudPainter(HUD2data *data, QWidget *parent = 0);
    void paint(QPainter *painter, QPaintEvent *event);
    void updateGeometry(QSize size);

signals:
    
public slots:

private:
    HudYawIndicator yaw;
    HudHorizon horizon;
    HUD2data *data;

    // HUD colors
    QColor defaultColor;       ///< Color for most HUD elements, e.g. pitch lines, center cross, change rate gauges
    QColor warningColor;       ///< Color for warning messages
    QColor criticalColor;      ///< Color for caution messages
    QColor infoColor;          ///< Color for normal/default messages
    QColor fuelColor;          ///< Current color for the fuel message, can be info, warning or critical color
};

#endif // HUDPAINTER_NEW_H

#ifndef HUDPAINTER_NEW_H
#define HUDPAINTER_NEW_H

#include <QWidget>
#include <QBrush>
#include <QFont>
#include <QPen>

#include "HUD2YawIndicator.h"
#include "HUD2Horizon.h"
#include "HUD2Data.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

class HUD2Painter : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Painter(HUD2data *huddata, QWidget *parent = 0);
    void paint(QPainter *painter, QPaintEvent *event);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void updateGeometry(const QSize *size);

private:
    HUD2YawIndicator yaw;
    HUD2Horizon horizon;
    HUD2data *huddata;

    // HUD colors
    QColor defaultColor;       ///< Color for most HUD elements, e.g. pitch lines, center cross, change rate gauges
    QColor warningColor;       ///< Color for warning messages
    QColor criticalColor;      ///< Color for caution messages
    QColor infoColor;          ///< Color for normal/default messages
    QColor fuelColor;          ///< Current color for the fuel message, can be info, warning or critical color
};

#endif // HUDPAINTER_NEW_H

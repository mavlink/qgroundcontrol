#ifndef HUD2DRAWER_H
#define HUD2DRAWER_H

#include <QWidget>
#include <QBrush>
#include <QFont>
#include <QPen>

#include "HUD2IndicatorHorizon.h"
#include "HUD2Data.h"
#include "HUD2Ribbon.h"
#include "HUD2IndicatorFps.h"
#include "HUD2IndicatorRoll.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

#define INSTRUMENTS_COLOR_DEFAULT   (Qt::green)
#define SKY_COLOR_DEFAULT           (Qt::darkBlue)
#define GND_COLOR_DEFAULT           (Qt::darkRed)

class HUD2Drawer : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Drawer(const HUD2Data *huddata, QWidget *parent);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);
    void paintComplete(void);

public slots:
    void updateGeometry(const QSize &size);
    void showDialog(void);
    void showColorDialog(void);

private:
    HUD2IndicatorHorizon horizon;
    HUD2IndicatorRoll roll;
    HUD2Ribbon speed;
    HUD2Ribbon climb;
    HUD2Ribbon compass;
    HUD2IndicatorFps fps;

    // HUD colors
    QColor defaultColor;       ///< Color for most HUD elements, e.g. pitch lines, center cross, change rate gauges
    QColor warningColor;       ///< Color for warning messages
    QColor criticalColor;      ///< Color for caution messages
    QColor infoColor;          ///< Color for normal/default messages
    QColor fuelColor;          ///< Current color for the fuel message, can be info, warning or critical color
};

#endif // HUD2DRAWER_H

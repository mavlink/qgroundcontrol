#ifndef HUD2DRAWER_H
#define HUD2DRAWER_H

#include <QWidget>
#include <QBrush>
#include <QFont>
#include <QPen>

#include "HUD2Horizon.h"
#include "HUD2Data.h"
#include "HUD2FpsIndicator.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QPaintEvent;
class QResizeEvent;
QT_END_NAMESPACE

class HUD2Drawer : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Drawer(HUD2Data &huddata, QWidget *parent);
    void paint_static(QPainter *painter);
    void paint_dynamic(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);
    void paintComplete(void);

public slots:
    void updateGeometry(const QSize &size);

private:
    HUD2Horizon horizon;
    HUD2FpsIndicator fps;
    HUD2Data &huddata;

    // HUD colors
    QColor defaultColor;       ///< Color for most HUD elements, e.g. pitch lines, center cross, change rate gauges
    QColor warningColor;       ///< Color for warning messages
    QColor criticalColor;      ///< Color for caution messages
    QColor infoColor;          ///< Color for normal/default messages
    QColor fuelColor;          ///< Current color for the fuel message, can be info, warning or critical color
};

#endif // HUD2DRAWER_H

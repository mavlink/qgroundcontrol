#ifndef MAV2DICON_H
#define MAV2DICON_H

#include <QGraphicsItem>

#include "UASInterface.h"
#include "opmapcontrol.h"

class MAV2DIcon : public mapcontrol::UAVItem
{
public:
    enum {
        MAV_ICON_GENERIC = 0,
        MAV_ICON_AIRPLANE,
        MAV_ICON_QUADROTOR,
        MAV_ICON_COAXIAL,
        MAV_ICON_HELICOPTER,
    } MAV_ICON_TYPE;

    //!
    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param radius the radius of the circle
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    MAV2DIcon(mapcontrol::MapGraphicItem* map,mapcontrol::OPMapWidget* parent, UASInterface* uas, int radius = 40, int type=0);

    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    MAV2DIcon(mapcontrol::MapGraphicItem* map,mapcontrol::OPMapWidget* parent, qreal lat=0, qreal lon=0, qreal alt=0, QColor color=QColor());

    virtual ~MAV2DIcon();

    /** @brief Mark this system as selected */
    void setSelectedUAS(bool selected);
    void setYaw(float yaw);
    /** @brief Set the airframe this MAV uses */
    void setAirframe(int airframe) {
        this->airframe = airframe;
    }

    /** @brief Get system id */
    int getUASId() const {
        return uasid;
    }

    void drawIcon();
    static void drawAirframePolygon(int airframe, QPainter& painter, int radius, QColor& iconColor, float yaw);

protected:
    float yaw;      ///< Yaw angle of the MAV
    int radius;     ///< Radius / width of the icon
    int type;       ///< Type of aircraft: 0: generic, 1: airplane, 2: quadrotor, 3-n: rotary wing
    int airframe;   ///< The specific type of airframe used
    QColor iconColor; ///< Color to be used for the icon
    bool selected;  ///< Wether this is the system currently in focus
    int uasid;      ///< ID of tracked system
    QSize size;

};

#endif // MAV2DICON_H

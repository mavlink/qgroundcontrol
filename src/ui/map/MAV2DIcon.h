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
    MAV2DIcon(mapcontrol::MapGraphicItem* map,mapcontrol::OPMapWidget* parent, UASInterface* uas, int radius = 10, int type=0);

    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    MAV2DIcon(mapcontrol::MapGraphicItem* map,mapcontrol::OPMapWidget* parent, qreal lat=0, qreal lon=0, qreal alt=0, QPen* pen=0);

    virtual ~MAV2DIcon();

    //! sets the QPen which is used for drawing the circle
    /*!
     * A QPen can be used to modify the look of the drawn circle
     * @param pen the QPen which should be used for drawing
     * @see http://doc.trolltech.com/4.3/qpen.html
     */
    virtual void setPen(QPen* pen);

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

    void drawIcon(QPen* pen);
    void drawIcon() {
        drawIcon(mypen);
    }
    static void drawAirframePolygon(int airframe, QPainter& painter, int radius, QColor& iconColor, float yaw);

protected:
    float yaw;      ///< Yaw angle of the MAV
    int radius;     ///< Radius / width of the icon
    int type;       ///< Type of aircraft: 0: generic, 1: airplane, 2: quadrotor, 3-n: rotary wing
    int airframe;   ///< The specific type of airframe used
    QColor iconColor; ///< Color to be used for the icon
    bool selected;  ///< Wether this is the system currently in focus
    int uasid;      ///< ID of tracked system
    QPen* mypen;
    QSize size;

};

#endif // MAV2DICON_H

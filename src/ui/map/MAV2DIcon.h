#ifndef MAV2DICON_H
#define MAV2DICON_H

#include <QGraphicsItem>
#include "qmapcontrol.h"

#include "UASInterface.h"

class MAV2DIcon : public qmapcontrol::Point
{
public:
    enum
    {
        MAV_ICON_GENERIC = 0,
        MAV_ICON_AIRPLANE,
        MAV_ICON_QUADROTOR,
        MAV_ICON_ROTARY_WING
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
    MAV2DIcon(UASInterface* uas, int radius = 10, int type=0, const QColor& color=QColor(Qt::red), QString name = QString(), Alignment alignment = Middle, QPen* pen=0);

    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    MAV2DIcon(qreal x, qreal y, QString name = QString(), Alignment alignment = Middle, QPen* pen=0);

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

    /** @brief Get system id */
    int getUASId() const { return uasid; }

    void drawIcon(QPen* pen);

protected:
    float yaw;      ///< Yaw angle of the MAV
    int radius;     ///< Radius / width of the icon
    int type;       ///< Type of aircraft: 0: generic, 1: airplane, 2: quadrotor, 3-n: rotary wing
    QColor iconColor; ///< Color to be used for the icon
    bool selected;  ///< Wether this is the system currently in focus
    int uasid;      ///< ID of tracked system

};

#endif // MAV2DICON_H

#ifndef MAV2DICON_H
#define MAV2DICON_H

#include "qmapcontrol.h"

class MAV2DIcon : public qmapcontrol::Point
{
public:
    /*!
     *
     * @param x longitude
     * @param y latitude
     * @param name name of the circle point
     * @param alignment alignment (Middle or TopLeft)
     * @param pen QPen for drawing
     */
    MAV2DIcon(qreal x, qreal y, QString name = QString(), Alignment alignment = Middle, QPen* pen=0);

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
    MAV2DIcon(qreal x, qreal y, int radius = 10, QString name = QString(), Alignment alignment = Middle, QPen* pen=0);
    virtual ~MAV2DIcon();

    //! sets the QPen which is used for drawing the circle
    /*!
     * A QPen can be used to modify the look of the drawn circle
     * @param pen the QPen which should be used for drawing
     * @see http://doc.trolltech.com/4.3/qpen.html
     */
    virtual void setPen(QPen* pen);

    void setYaw(float yaw);
    void drawIcon(QPen* pen);

protected:
    float yaw;  ///< Yaw angle of the MAV
    int radius; ///< Maximum dimension of the MAV icon


};

#endif // MAV2DICON_H

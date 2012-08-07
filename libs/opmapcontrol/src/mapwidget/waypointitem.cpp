/**
******************************************************************************
*
* @file       waypointitem.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A graphicsItem representing a WayPoint
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
*
*****************************************************************************/
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "waypointitem.h"
namespace mapcontrol
{
    WayPointItem::WayPointItem(const internals::PointLatLng &coord,double const& altitude, MapGraphicItem *map) :
        map(map),
        autoreachedEnabled(true),
        coord(coord),
        reached(false),
        description(""),
        shownumber(true),
        isDragging(false),
        altitude(altitude),
        heading(0)
    {
        text=0;
        numberI=0;
        picture.load(QString::fromUtf8(":/markers/images/marker.png"));
        number=WayPointItem::snumber;
        ++WayPointItem::snumber;
        this->setFlag(QGraphicsItem::ItemIsMovable,true);
        this->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
        this->setFlag(QGraphicsItem::ItemIsSelectable,true);
       // transf.translate(picture.width()/2,picture.height());
       // this->setTransform(transf);
        SetShowNumber(shownumber);
        RefreshToolTip();
        RefreshPos();
    }
    WayPointItem::WayPointItem(const internals::PointLatLng &coord,double const& altitude, const QString &description, MapGraphicItem *map) :
        map(map),
        coord(coord),
        reached(false),
        description(description),
        shownumber(true),
        isDragging(false),
        altitude(altitude),
        heading(0)
    {
        text=0;
        numberI=0;
        picture.load(QString::fromUtf8(":/markers/images/marker.png"));
        number=WayPointItem::snumber;
        ++WayPointItem::snumber;
        this->setFlag(QGraphicsItem::ItemIsMovable,true);
        this->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
        this->setFlag(QGraphicsItem::ItemIsSelectable,true);
       //transf.translate(picture.width()/2,picture.height());
       // this->setTransform(transf);
        SetShowNumber(shownumber);
        RefreshToolTip();
        RefreshPos();
    }

    QRectF WayPointItem::boundingRect() const
    {
        return QRectF(-picture.width()/2,-picture.height(),picture.width(),picture.height());
    }

    void WayPointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->drawPixmap(-picture.width()/2,-picture.height(),picture);
        if(this->isSelected())
        {
            painter->drawRect(QRectF(-picture.width()/2,-picture.height(),picture.width()-1,picture.height()-1));
        }
    }
    void WayPointItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        if(event->button()==Qt::LeftButton)
        {
	    text=new QGraphicsSimpleTextItem(this);
            textBG=new QGraphicsRectItem(this);

//	    textBG->setBrush(Qt::white);
//	    textBG->setOpacity(0.5);

	    textBG->setBrush(QColor(255, 255, 255, 128));

	    text->setPen(QPen(Qt::red));
	    text->setPos(10,-picture.height());
	    textBG->setPos(10,-picture.height());
	    text->setZValue(3);
	    RefreshToolTip();
	    isDragging=true;
	}
        QGraphicsItem::mousePressEvent(event);
    }
    void WayPointItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        if(event->button()==Qt::LeftButton)
        {
            delete text;
            delete textBG;
            coord=map->FromLocalToLatLng(this->pos().x(),this->pos().y());
            QString coord_str = " " + QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);
            // qDebug() << "WP MOVE:" << coord_str << __FILE__ << __LINE__;
            isDragging=false;
            RefreshToolTip();

            emit WPValuesChanged(this);
        }
        QGraphicsItem::mouseReleaseEvent(event);
    }
    void WayPointItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {

        if(isDragging)
        {
            coord=map->FromLocalToLatLng(this->pos().x(),this->pos().y());
            QString coord_str = " " + QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);
            text->setText(coord_str);
            // qDebug() << "WP DRAG:" << coord_str << __FILE__ << __LINE__;
            textBG->setRect(text->boundingRect());

            emit WPValuesChanged(this);
        }
            QGraphicsItem::mouseMoveEvent(event);
    }
    void WayPointItem::SetAltitude(const double &value)
    {
        altitude=value;
        RefreshToolTip();

        emit WPValuesChanged(this);
        this->update();
    }
    void WayPointItem::SetHeading(const float &value)
    {
        heading=value;
        RefreshToolTip();

        emit WPValuesChanged(this);
        this->update();
    }
    void WayPointItem::SetCoord(const internals::PointLatLng &value)
    {
        coord=value;
        emit WPValuesChanged(this);
        RefreshPos();
        RefreshToolTip();
        this->update();
    }
    void WayPointItem::SetDescription(const QString &value)
    {
        description=value;
        RefreshToolTip();
        emit WPValuesChanged(this);
        this->update();
    }
    void WayPointItem::SetNumber(const int &value)
    {
        emit WPNumberChanged(number,value,this);
        number=value;
        RefreshToolTip();
        numberI->setText(QString::number(number));
        numberIBG->setRect(numberI->boundingRect().adjusted(-2,0,1,0));
        this->update();
    }
    void WayPointItem::SetReached(const bool &value)
    {
        if (autoreachedEnabled)
        {
            reached=value;
            emit WPValuesChanged(this);
            if(value)
                picture.load(QString::fromUtf8(":/markers/images/bigMarkerGreen.png"));
            else
                picture.load(QString::fromUtf8(":/markers/images/marker.png"));
            this->update();
        }
    }
    void WayPointItem::SetShowNumber(const bool &value)
    {
//        shownumber=value;
//        if((numberI==0) && value)
//        {
//            numberI=new QGraphicsSimpleTextItem(this);
//            numberIBG=new QGraphicsRectItem(this);
//            numberIBG->setBrush(Qt::white);
//            numberIBG->setOpacity(0.5);
//            numberI->setZValue(3);
//            numberI->setPen(QPen(Qt::blue));
//            numberI->setPos(0,-13-picture.height());
//            numberIBG->setPos(0,-13-picture.height());
//            numberI->setText(QString::number(number));
//            numberIBG->setRect(numberI->boundingRect().adjusted(-2,0,1,0));
//        }
//        else if (!value && numberI)
//        {
//            delete numberI;
//            delete numberIBG;
//        }
//        this->update();



        shownumber=value;
        if((numberI==0) && value)
        {
            numberI=new QGraphicsSimpleTextItem(this);
            numberIBG=new QGraphicsRectItem(this);
            numberIBG->setBrush(Qt::black);
            numberIBG->setOpacity(0.5);
            numberI->setZValue(3);
            numberI->setPen(QPen(Qt::white));
            numberI->setPos(18,-picture.height()/2-2);
            numberIBG->setPos(18,-picture.height()/2-2);
            numberI->setText(QString::number(number));
            numberIBG->setRect(numberI->boundingRect().adjusted(-2,0,1,0));
        }
        else if (!value && numberI)
        {
            delete numberI;
            delete numberIBG;
        }
        this->update();



    }
    void WayPointItem::WPDeleted(const int &onumber)
    {
        if(number>onumber) --number;
        numberI->setText(QString::number(number));
        numberIBG->setRect(numberI->boundingRect().adjusted(-2,0,1,0));
        RefreshToolTip();
        this->update();
    }
    void WayPointItem::WPInserted(const int &onumber, WayPointItem *waypoint)
    {
        if(waypoint!=this)
        {
            if(onumber<=number) ++number;
            numberI->setText(QString::number(number));
            RefreshToolTip();
            this->update();
        }
    }
    void WayPointItem::WPRenumbered(const int &oldnumber, const int &newnumber, WayPointItem *waypoint)
    {
        if (waypoint!=this)
        {
            if(((oldnumber>number) && (newnumber<=number)))
            {
                ++number;
                numberI->setText(QString::number(number));
                numberIBG->setRect(numberI->boundingRect().adjusted(-2,0,1,0));
                RefreshToolTip();
            }
            else if (((oldnumber<number) && (newnumber>number)))
            {
                --number;
                numberI->setText(QString::number(number));
                numberIBG->setRect(numberI->boundingRect().adjusted(-2,0,1,0));
                RefreshToolTip();
            }
            else if (newnumber==number)
            {
                ++number;
                numberI->setText(QString::number(number));
                RefreshToolTip();
            }
            this->update();
        }
    }
    int WayPointItem::type() const
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    }

    WayPointItem::~WayPointItem()
    {
        --WayPointItem::snumber;
    }
    void WayPointItem::RefreshPos()
    {
        core::Point point=map->FromLatLngToLocal(coord);
        this->setPos(point.X(),point.Y());
    }
    void WayPointItem::RefreshToolTip()
    {
        QString coord_str = QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);
        setToolTip(QString("WayPoint Number: %1\nDescription: %2\nCoordinate: %4\nAltitude: %5 m (MSL)\nHeading: %6 deg").arg(QString::number(WayPointItem::number)).arg(description).arg(coord_str).arg(QString::number(altitude)).arg(QString::number(heading)));
    }

    int WayPointItem::snumber=0;
}

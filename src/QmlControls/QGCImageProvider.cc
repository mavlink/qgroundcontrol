/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

QGROUNDCONTROL is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QGROUNDCONTROL is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
* @file
*   @brief Image Provider
*
*   @author Gus Grubba <mavlink@grubba.com>
*
*/


#include "QGCImageProvider.h"

#include <QPainter>
#include <QFont>

QGCImageProvider::QGCImageProvider(QGCApplication *app)
    : QGCTool(app)
    , QQuickImageProvider(QQmlImageProviderBase::Image)
{
}

QGCImageProvider::~QGCImageProvider()
{

}

void QGCImageProvider::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   //-- Dummy temporary image until something comes along
   _pImage = QImage(320, 240, QImage::Format_RGBA8888);
   _pImage.fill(Qt::black);
   QPainter painter(&_pImage);
   QFont f = painter.font();
   f.setPixelSize(20);
   painter.setFont(f);
   painter.setPen(Qt::white);
   painter.drawText(QRectF(0, 0, 320, 240), Qt::AlignCenter, "Waiting...");
}

QImage QGCImageProvider::requestImage(const QString & /* image url with vehicle id*/, QSize *, const QSize &)
{
/*
    The QML side will request an image using a special URL, which we've registered as QGCImages.
    The URL follows this format (or anything you want to make out of it after the "QGCImages" part):

    "image://QGCImages/vvv/iii"

    Where:
        vvv: Some vehicle id
        iii: An auto incremented index (which forces the Item to reload the image)

    The image index is incremented each time a new image arrives. A signal is emitted and the QML side
    updates its contents automatically.

        Image {
            source:     "image://QGCImages/" + _activeVehicle.id + "/" + _activeVehicle.flowImageIndex
            width:      parent.width * 0.5
            height:     width * 0.75
            cache:      false
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
        }

    For now, we don't even look at the URL. This will have to be fixed if we're to support multiple
    vehicles transmitting flow images.
*/
    return _pImage;
}

void QGCImageProvider::setImage(QImage* pImage, int /* vehicle id*/)
{
    _pImage = pImage->mirrored();
}


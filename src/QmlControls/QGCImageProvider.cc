/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
* @file
*   @brief Image Provider
*
*   @author Gus Grubba <gus@auterion.com>
*
*/


#include "QGCImageProvider.h"

#include <QPainter>
#include <QFont>

QGCImageProvider::QGCImageProvider(QGCApplication *app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
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


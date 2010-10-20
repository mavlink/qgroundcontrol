#include "WebImage.h"

#include <QDebug>
#include <QGLWidget>

WebImage::WebImage()
    : state(WebImage::UNINITIALIZED)
    , sourceURL("")
    , image(new QImage)
    , lastReference(0)
    , syncFlag(false)
{

}

void
WebImage::clear(void)
{
    image.reset();
    sourceURL.clear();
    state = WebImage::UNINITIALIZED;
    lastReference = 0;
}

WebImage::State
WebImage::getState(void) const
{
    return state;
}

void
WebImage::setState(State state)
{
    this->state = state;
}

const QString&
WebImage::getSourceURL(void) const
{
    return sourceURL;
}

void
WebImage::setSourceURL(const QString& url)
{
    sourceURL = url;
}

const uint8_t*
WebImage::getData(void) const
{
    return image->scanLine(0);
}

void
WebImage::setData(const QByteArray& data)
{
    QImage tempImage;
    if (tempImage.loadFromData(data))
    {
        if (image.isNull())
        {
            image.reset(new QImage);
        }
        *image = QGLWidget::convertToGLFormat(tempImage);
    }
    else
    {
        qDebug() << "# WARNING: cannot load image data for" << sourceURL;
    }
}

int32_t
WebImage::getWidth(void) const
{
    return image->width();
}

int32_t
WebImage::getHeight(void) const
{
    return image->height();
}

int32_t
WebImage::getByteCount(void) const
{
    return image->byteCount();
}

uint64_t
WebImage::getLastReference(void) const
{
    return lastReference;
}

void
WebImage::setLastReference(uint64_t value)
{
    lastReference = value;
}

bool
WebImage::getSyncFlag(void) const
{
    return syncFlag;
}

void
WebImage::setSyncFlag(bool onoff)
{
    syncFlag = onoff;
}

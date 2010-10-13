#include "WebImage.h"

WebImage::WebImage()
    : state(WebImage::UNINITIALIZED)
    , lastReference(0)
    , syncFlag(false)
{

}

void
WebImage::clear(void)
{
    image.clear();
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

QString
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
    return image->bits();
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

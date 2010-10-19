#ifndef WEBIMAGE_H
#define WEBIMAGE_H

#include <inttypes.h>
#include <QImage>
#include <QScopedPointer>
#include <QSharedPointer>

class WebImage
{
public:
    WebImage();

    void clear(void);

    enum State
    {
        UNINITIALIZED = 0,
        REQUESTED = 1,
        READY = 2
    };

    State getState(void) const;
    void setState(State state);

    const QString& getSourceURL(void) const;
    void setSourceURL(const QString& url);

    const uint8_t* getData(void) const;
    void setData(const QByteArray& data);

    int32_t getWidth(void) const;
    int32_t getHeight(void) const;
    int32_t getByteCount(void) const;

    uint64_t getLastReference(void) const;
    void setLastReference(uint64_t value);

    bool getSyncFlag(void) const;
    void setSyncFlag(bool onoff);

private:
    State state;
    QString sourceURL;
    QSharedPointer<QImage> image;
    uint64_t lastReference;
    bool syncFlag;
};

typedef QSharedPointer<WebImage> WebImagePtr;

#endif // WEBIMAGE_H

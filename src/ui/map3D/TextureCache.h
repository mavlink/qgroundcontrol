#ifndef TEXTURECACHE_H
#define TEXTURECACHE_H

#include <QVector>

#include "Texture.h"
#include "WebImageCache.h"

class TextureCache
{
public:
    explicit TextureCache(uint32_t cacheSize);

    TexturePtr get(const QString& tileURL);

    void sync(void);

private:
    QPair<TexturePtr, int32_t> lookup(const QString& tileURL);

    bool requireSync(void) const;

    uint32_t cacheSize;
    QVector<TexturePtr> textures;

    QScopedPointer<WebImageCache> imageCache;
};

#endif // TEXTURECACHE_H

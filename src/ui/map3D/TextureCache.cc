#include "TextureCache.h"

TextureCache::TextureCache(uint32_t _cacheSize)
    : cacheSize(_cacheSize)
    , imageCache(new WebImageCache(0, cacheSize))
{
    textures.resize(cacheSize);
    TexturePtr t;
    foreach(t, textures)
    {
        GLuint id;
        glGenTextures(1, &id);
        t->setID(id);
        glBindTexture(GL_TEXTURE_2D, id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

TexturePtr
TextureCache::get(const QString& tileURL)
{
    QPair<TexturePtr, int32_t> p1 = lookup(tileURL);
    if (!p1.first.isNull())
    {
        return p1.first;
    }

    QPair<WebImagePtr, int32_t> p2 = imageCache->lookup(tileURL);
    if (!p2.first.isNull())
    {
        textures[p2.second]->sync(p2.first);
        p1 = lookup(tileURL);

        return p1.first;
    }

    return TexturePtr();
}

void
TextureCache::sync(void)
{
    if (requireSync())
    {
        for (int32_t i = 0; i < textures.size(); ++i)
        {
            textures[i]->sync(imageCache->at(i));
        }
    }
}

QPair<TexturePtr, int32_t>
TextureCache::lookup(const QString& tileURL)
{
    for (int32_t i = 0; i < textures.size(); ++i)
    {
        if (textures[i]->getSourceURL() == tileURL)
        {
            return qMakePair(textures[i], i);
        }
    }

    return qMakePair(TexturePtr(), -1);
}

bool
TextureCache::requireSync(void) const
{
    for (uint32_t i = 0; i < cacheSize; ++i)
    {
        if (imageCache->at(i)->getSyncFlag())
        {
            return true;
        }
    }
    return false;
}

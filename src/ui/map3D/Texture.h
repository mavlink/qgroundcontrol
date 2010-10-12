#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/gl.h>
#include <QSharedPointer>

class Texture
{
public:
    Texture();

    enum State
    {
        UNINITIALIZED = 0,
        REQUESTED = 1,
        READY = 2
    };

    State getState(void) const;
    GLuint getTextureId(void) const;
    int32_t getTextureWidth(void) const;
    int32_t getTextureHeight(void) const;
    int32_t getImageWidth(void) const;
    int32_t getImageHeight(void) const;
    float getMaxU(void) const;
    float getMaxV(void) const;

private:
    State state;

    GLuint textureId;

    int32_t textureWidth;
    int32_t textureHeight;

    int32_t imageWidth;
    int32_t imageHeight;

    float maxU;
    float maxV;
};

typedef struct QSharedPointer<Texture> TexturePtr;

#endif // TEXTURE_H

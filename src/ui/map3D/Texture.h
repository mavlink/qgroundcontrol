#ifndef TEXTURE_H
#define TEXTURE_H

#if (defined __APPLE__) & (defined __MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <inttypes.h>
#include <QSharedPointer>

class Texture
{
public:
    Texture();

    void draw(float x1, float y1, float x2, float y2,
              bool smoothInterpolation) const;

private:
    enum State
    {
        UNINITIALIZED = 0,
        REQUESTED = 1,
        READY = 2
    };

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

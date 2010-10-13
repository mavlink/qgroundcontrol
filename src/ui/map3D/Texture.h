#ifndef TEXTURE_H
#define TEXTURE_H

#if (defined __APPLE__) & (defined __MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <inttypes.h>
#include <QSharedPointer>

#include "WebImage.h"

class Texture
{
public:
    Texture();

    QString getSourceURL(void);

    void setID(GLuint id);

    void sync(const WebImagePtr& image);

    void draw(float x1, float y1, float x2, float y2,
              bool smoothInterpolation) const;
    void draw(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              bool smoothInterpolation) const;

private:
    enum State
    {
        UNINITIALIZED = 0,
        REQUESTED = 1,
        READY = 2
    };

    State state;
    QString sourceURL;
    GLuint id;

    int32_t textureWidth;
    int32_t textureHeight;

    int32_t imageWidth;
    int32_t imageHeight;

    float maxU;
    float maxV;
};

typedef QSharedPointer<Texture> TexturePtr;

#endif // TEXTURE_H

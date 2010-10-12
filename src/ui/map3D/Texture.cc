#include "Texture.h"

Texture::Texture()
{

}

Texture::State
Texture::getState(void) const
{
    return state;
}

GLuint
Texture::getTextureId(void) const
{
    return textureId;
}

int32_t
Texture::getTextureWidth(void) const
{
    return textureWidth;
}

int32_t
Texture::getTextureHeight(void) const
{
    return textureHeight;
}

int32_t
Texture::getImageWidth(void) const
{
    return imageWidth;
}

int32_t
Texture::getImageHeight(void) const
{
    return imageHeight;
}

float
Texture::getMaxU(void) const
{
    return maxU;
}

float
Texture::getMaxV(void) const
{
    return maxV;
}

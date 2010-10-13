#include "Texture.h"

Texture::Texture()
{

}

QString
Texture::getSourceURL(void)
{
    return sourceURL;
}

void
Texture::setID(GLuint id)
{
    this->id = id;
}

void
Texture::sync(const WebImagePtr& image)
{
    state = static_cast<State>(image->getState());

    if (image->getState() != WebImage::UNINITIALIZED &&
        sourceURL != image->getSourceURL())
    {
        sourceURL = image->getSourceURL();
    }

    if (image->getState() == WebImage::READY && image->getSyncFlag())
    {
        image->setSyncFlag(false);

        if (image->getWidth() != imageWidth ||
            image->getHeight() != imageHeight)
        {
            imageWidth = image->getWidth();
            textureWidth = 32;
            while (textureWidth < imageWidth)
            {
                textureWidth *= 2;
            }
            imageHeight = image->getHeight();
            textureHeight = 32;
            while (textureHeight < imageHeight)
            {
                textureHeight *= 2;
            }

            maxU = static_cast<double>(imageWidth)
                   / static_cast<double>(textureWidth);
            maxV = static_cast<double>(imageHeight)
                   / static_cast<double>(textureHeight);

            glBindTexture(GL_TEXTURE_2D, id);
            glTexImage2D(GL_TEXTURE_2D, 0, 3, textureWidth, textureHeight,
                         0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        }

        glBindTexture(GL_TEXTURE_2D, id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight,
                        GL_RGB, GL_UNSIGNED_BYTE, image->getData());

    }
}

void
Texture::draw(float x1, float y1, float x2, float y2,
              bool smoothInterpolation) const
{
    if (state == REQUESTED)
    {
        glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, id);

    float dx, dy;
    if (smoothInterpolation)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        dx = 1.0f / (2.0f * textureWidth);
        dy = 1.0f / (2.0f * textureHeight);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        dx = 0.0f;
        dy = 0.0f;
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);

    glTexCoord2f(dx, maxV - dy);
    glVertex2f(x1, y1);
    glTexCoord2f(maxU - dx, maxV - dy);
    glVertex2f(x2, y1);
    glTexCoord2f(maxU - dx, dy);
    glVertex2f(x2, y2);
    glTexCoord2f(dx, dy);
    glVertex2f(x1, y2);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void
Texture::draw(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              bool smoothInterpolation) const
{
    if (state == REQUESTED)
    {
        glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
        glEnd();
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, id);

    float dx, dy;
    if (smoothInterpolation)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        dx = 1.0f / (2.0f * textureWidth);
        dy = 1.0f / (2.0f * textureHeight);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        dx = 0.0f;
        dy = 0.0f;
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);

    glTexCoord2f(dx, maxV - dy);
    glVertex2f(x1, y1);
    glTexCoord2f(maxU - dx, maxV - dy);
    glVertex2f(x2, y2);
    glTexCoord2f(maxU - dx, dy);
    glVertex2f(x3, y3);
    glTexCoord2f(dx, dy);
    glVertex2f(x4, y4);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

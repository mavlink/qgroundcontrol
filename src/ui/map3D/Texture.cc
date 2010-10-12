#include "Texture.h"

Texture::Texture()
{

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
    glBindTexture(GL_TEXTURE_2D, textureId);

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

#include "GLOverlayGeode.h"

GLOverlayGeode::GLOverlayGeode()
 : mDrawable(new GLOverlayDrawable)
 , mMessageTimestamp(0.0)
{
    setCullingActive(false);

    addDrawable(mDrawable);
}

void
GLOverlayGeode::setOverlay(px::GLOverlay &overlay)
{
    mDrawable->setOverlay(overlay);
    mCoordinateFrameType = overlay.coordinateframetype();

    dirtyBound();
}

px::GLOverlay::CoordinateFrameType
GLOverlayGeode::coordinateFrameType(void) const
{
    return mCoordinateFrameType;
}

void
GLOverlayGeode::setMessageTimestamp(qreal timestamp)
{
    mMessageTimestamp = timestamp;
}

qreal
GLOverlayGeode::messageTimestamp(void) const
{
    return mMessageTimestamp;
}

GLOverlayGeode::GLOverlayDrawable::GLOverlayDrawable()
{
    setUseDisplayList(false);
    setUseVertexBufferObjects(true);
}

GLOverlayGeode::GLOverlayDrawable::GLOverlayDrawable(const GLOverlayDrawable& drawable,
                                                     const osg::CopyOp& copyop)
 : osg::Drawable(drawable,copyop)
{
    setUseDisplayList(false);
    setUseVertexBufferObjects(true);
}

void
GLOverlayGeode::GLOverlayDrawable::setOverlay(px::GLOverlay &overlay)
{
    if (!overlay.IsInitialized())
    {
        return;
    }

    mOverlay = overlay;

    mBBox.init();

    const std::string& data = mOverlay.data();

    for (size_t i = 0; i < data.size(); ++i)
    {
        switch (data.at(i)) {
        case px::GLOverlay::POINTS:
            break;
        case px::GLOverlay::LINES:
            break;
        case px::GLOverlay::LINE_STRIP:
            break;
        case px::GLOverlay::LINE_LOOP:
            break;
        case px::GLOverlay::TRIANGLES:
            break;
        case px::GLOverlay::TRIANGLE_STRIP:
            break;
        case px::GLOverlay::TRIANGLE_FAN:
            break;
        case px::GLOverlay::QUADS:
            break;
        case px::GLOverlay::QUAD_STRIP:
            break;
        case px::GLOverlay::POLYGON:
            break;
        case px::GLOverlay::WIRE_CIRCLE:
            i += sizeof(float) * 4;
            break;
        case px::GLOverlay::SOLID_CIRCLE:
            i += sizeof(float) * 4;
            break;
        case px::GLOverlay::SOLID_CUBE:
            i += sizeof(float) * 5;
            break;
        case px::GLOverlay::WIRE_CUBE:
            i += sizeof(float) * 5;
            break;
        case px::GLOverlay::END:
            break;
        case px::GLOverlay::VERTEX2F:
            i += sizeof(float) * 2;
            break;
        case px::GLOverlay::VERTEX3F:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);

            mBBox.expandBy(x, y, z);
        }
            break;
        case px::GLOverlay::ROTATEF:
            i += sizeof(float) * 4;
            break;
        case px::GLOverlay::TRANSLATEF:
            i += sizeof(float) * 3;
            break;
        case px::GLOverlay::SCALEF:
            i += sizeof(float) * 3;
            break;
        case px::GLOverlay::PUSH_MATRIX:
            break;
        case px::GLOverlay::POP_MATRIX:
            break;
        case px::GLOverlay::COLOR3F:
            i += sizeof(float) * 3;
            break;
        case px::GLOverlay::COLOR4F:
            i += sizeof(float) * 4;
            break;
        case px::GLOverlay::POINTSIZE:
            i += sizeof(float);
            break;
        case px::GLOverlay::LINEWIDTH:
            i += sizeof(float);
            break;
        }
    }
}

void
GLOverlayGeode::GLOverlayDrawable::drawImplementation(osg::RenderInfo&) const
{
    if (!mOverlay.IsInitialized())
    {
        return;
    }

    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();

    glScalef(-1.0f, 1.0f, -1.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);

    const std::string& data = mOverlay.data();

    for (size_t i = 0; i < data.size(); ++i)
    {
        switch (data.at(i)) {
        case px::GLOverlay::POINTS:
            glBegin(GL_POINTS);
            break;
        case px::GLOverlay::LINES:
            glBegin(GL_LINES);
            break;
        case px::GLOverlay::LINE_STRIP:
            glBegin(GL_LINE_STRIP);
            break;
        case px::GLOverlay::LINE_LOOP:
            glBegin(GL_LINE_LOOP);
            break;
        case px::GLOverlay::TRIANGLES:
            glBegin(GL_TRIANGLES);
            break;
        case px::GLOverlay::TRIANGLE_STRIP:
            glBegin(GL_TRIANGLE_STRIP);
            break;
        case px::GLOverlay::TRIANGLE_FAN:
            glBegin(GL_TRIANGLE_FAN);
            break;
        case px::GLOverlay::QUADS:
            glBegin(GL_QUADS);
            break;
        case px::GLOverlay::QUAD_STRIP:
            glBegin(GL_QUAD_STRIP);
            break;
        case px::GLOverlay::POLYGON:
            glBegin(GL_POLYGON);
            break;
        case px::GLOverlay::WIRE_CIRCLE:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            float r = getFloatValue(data, i);

            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 20; i++)
            {
                float angle = i / 20.0f * M_PI * 2.0f;
                glVertex3f(x + r * cosf(angle), y + r * sinf(angle), z);
            }
            glEnd();
        }
            break;
        case px::GLOverlay::SOLID_CIRCLE:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            float r = getFloatValue(data, i);

            glBegin(GL_POLYGON);
            for (int i = 0; i < 20; i++)
            {
                float angle = i / 20.0f * M_PI * 2.0f;
                glVertex3f(x + r * cosf(angle), y + r * sinf(angle), z);
            }
            glEnd();
        }
            break;
        case px::GLOverlay::SOLID_CUBE:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            float w = getFloatValue(data, i);
            float h = getFloatValue(data, i);

            float w_2 = w / 2.0f;
            float h_2 = h / 2.0f;
            glBegin(GL_QUADS);
            // face 1
            glNormal3f(1.0f, 0.0f, 0.0f);
            glVertex3f(x + w_2, y - w_2, z + h_2);
            glVertex3f(x + w_2, y - w_2, z - h_2);
            glVertex3f(x + w_2, y + w_2, z - h_2);
            glVertex3f(x + w_2, y + w_2, z + h_2);
            // face 2
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(x + w_2, y + w_2, z + h_2);
            glVertex3f(x + w_2, y + w_2, z - h_2);
            glVertex3f(x - w_2, y + w_2, z - h_2);
            glVertex3f(x - w_2, y + w_2, z + h_2);
            // face 3
            glNormal3f(0.0f, 0.0f, 1.0f);
            glVertex3f(x + w_2, y + w_2, z + h_2);
            glVertex3f(x - w_2, y + w_2, z + h_2);
            glVertex3f(x - w_2, y - w_2, z + h_2);
            glVertex3f(x + w_2, y - w_2, z + h_2);
            // face 4
            glNormal3f(-1.0f, 0.0f, 0.0f);
            glVertex3f(x - w_2, y - w_2, z + h_2);
            glVertex3f(x - w_2, y + w_2, z + h_2);
            glVertex3f(x - w_2, y + w_2, z - h_2);
            glVertex3f(x - w_2, y - w_2, z - h_2);
            // face 5
            glNormal3f(0.0f, -1.0f, 0.0f);
            glVertex3f(x - w_2, y - w_2, z + h_2);
            glVertex3f(x - w_2, y - w_2, z - h_2);
            glVertex3f(x + w_2, y - w_2, z - h_2);
            glVertex3f(x + w_2, y - w_2, z + h_2);
            // face 6
            glNormal3f(0.0f, 0.0f, -1.0f);
            glVertex3f(x - w_2, y - w_2, z - h_2);
            glVertex3f(x - w_2, y + w_2, z - h_2);
            glVertex3f(x + w_2, y + w_2, z - h_2);
            glVertex3f(x + w_2, y - w_2, z - h_2);

            glEnd();
        }
            break;
        case px::GLOverlay::WIRE_CUBE:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            float w = getFloatValue(data, i);
            float h = getFloatValue(data, i);

            float w_2 = w / 2.0f;
            float h_2 = h / 2.0f;
            // face 1
            glBegin(GL_LINE_LOOP);
            glVertex3f(x + w_2, y - w_2, z + h_2);
            glVertex3f(x + w_2, y - w_2, z - h_2);
            glVertex3f(x + w_2, y + w_2, z - h_2);
            glVertex3f(x + w_2, y + w_2, z + h_2);
            glEnd();
            // face 2
            glBegin(GL_LINE_LOOP);
            glVertex3f(x + w_2, y + w_2, z + h_2);
            glVertex3f(x + w_2, y + w_2, z - h_2);
            glVertex3f(x - w_2, y + w_2, z - h_2);
            glVertex3f(x - w_2, y + w_2, z + h_2);
            glEnd();
            // face 3
            glBegin(GL_LINE_LOOP);
            glVertex3f(x + w_2, y + w_2, z + h_2);
            glVertex3f(x - w_2, y + w_2, z + h_2);
            glVertex3f(x - w_2, y - w_2, z + h_2);
            glVertex3f(x + w_2, y - w_2, z + h_2);
            glEnd();
            // face 4
            glBegin(GL_LINE_LOOP);
            glVertex3f(x - w_2, y - w_2, z + h_2);
            glVertex3f(x - w_2, y + w_2, z + h_2);
            glVertex3f(x - w_2, y + w_2, z - h_2);
            glVertex3f(x - w_2, y - w_2, z - h_2);
            glEnd();
            // face 5
            glBegin(GL_LINE_LOOP);
            glVertex3f(x - w_2, y - w_2, z + h_2);
            glVertex3f(x - w_2, y - w_2, z - h_2);
            glVertex3f(x + w_2, y - w_2, z - h_2);
            glVertex3f(x + w_2, y - w_2, z + h_2);
            glEnd();
            // face 6
            glBegin(GL_LINE_LOOP);
            glVertex3f(x - w_2, y - w_2, z - h_2);
            glVertex3f(x - w_2, y + w_2, z - h_2);
            glVertex3f(x + w_2, y + w_2, z - h_2);
            glVertex3f(x + w_2, y - w_2, z - h_2);
            glEnd();
        }
            break;
        case px::GLOverlay::END:
            glEnd();
            break;
        case px::GLOverlay::VERTEX2F:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            glVertex2f(x, y);
        }
            break;
        case px::GLOverlay::VERTEX3F:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            glVertex3f(x, y, z);
        }
            break;
        case px::GLOverlay::ROTATEF:
        {
            float angle = getFloatValue(data, i);
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            glRotatef(angle, x, y, z);
        }
            break;
        case px::GLOverlay::TRANSLATEF:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            glTranslatef(x, y, z);
        }
            break;
        case px::GLOverlay::SCALEF:
        {
            float x = getFloatValue(data, i);
            float y = getFloatValue(data, i);
            float z = getFloatValue(data, i);
            glScalef(x, y, z);
        }
            break;
        case px::GLOverlay::PUSH_MATRIX:
            glPushMatrix();
            break;
        case px::GLOverlay::POP_MATRIX:
            glPopMatrix();
            break;
        case px::GLOverlay::COLOR3F:
        {
            float red = getFloatValue(data, i);
            float green = getFloatValue(data, i);
            float blue = getFloatValue(data, i);
            glColor3f(red, green, blue);
        }
            break;
        case px::GLOverlay::COLOR4F:
        {
            float red = getFloatValue(data, i);
            float green = getFloatValue(data, i);
            float blue = getFloatValue(data, i);
            float alpha = getFloatValue(data, i);
            glColor4f(red, green, blue, alpha);
        }
            break;
        case px::GLOverlay::POINTSIZE:
            glPointSize(getFloatValue(data, i));
            break;
        case px::GLOverlay::LINEWIDTH:
            glLineWidth(getFloatValue(data, i));
            break;
        }
    }

    glPopMatrix();
    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

osg::BoundingBox
GLOverlayGeode::GLOverlayDrawable::computeBound() const
{
    return mBBox;
}

float
GLOverlayGeode::GLOverlayDrawable::getFloatValue(const std::string& data,
                                                 size_t& mark) const
{
    char temp[4];
    for (int i = 0; i < 4; ++i)
    {
        ++mark;
        temp[i] = data.at(mark);
    }

    char *cp = &(temp[0]);
    float *fp = reinterpret_cast<float *>(cp);

    return *fp;
}

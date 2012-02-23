#ifndef GLOVERLAYGEODE_H
#define GLOVERLAYGEODE_H

#include <mavlink_protobuf_manager.hpp>
#include <osg/Geode>

class GLOverlayGeode : public osg::Geode
{
public:
    GLOverlayGeode();

    void setOverlay(px::GLOverlay& overlay);

    px::GLOverlay::CoordinateFrameType coordinateFrameType(void) const;

private:
    class GLOverlayDrawable : public osg::Drawable
    {
    public:
        GLOverlayDrawable();

        GLOverlayDrawable(const GLOverlayDrawable& drawable,
                          const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        void setOverlay(px::GLOverlay& overlay);

        META_Object(GLOverlayDrawableApp, GLOverlayDrawable)

        virtual void drawImplementation(osg::RenderInfo&) const;

        virtual osg::BoundingBox computeBound() const;

    private:
        float getFloatValue(const std::string& data, size_t& mark) const;

        px::GLOverlay mOverlay;
        osg::BoundingBox mBBox;
    };

    osg::ref_ptr<GLOverlayDrawable> mDrawable;
    px::GLOverlay::CoordinateFrameType mCoordinateFrameType;
};

#endif // GLOVERLAYGEODE_H

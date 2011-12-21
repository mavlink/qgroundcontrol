// -*-c++-*-

/*
 * OpenSceneGraph - Copyright (C) 1998-2009 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
 */

/*
 * osgFX::Outline - Copyright (C) 2004,2009 Ulrich Hertlein
 */

#ifndef OSGFX_OUTLINE_
#define OSGFX_OUTLINE_

#include <osgFX/Export>
#include <osgFX/Effect>

namespace osgFX
{
    /**
     * Outline effect.
     * This effect draws a stencil buffer-based outline around an object.
     * Color and width of the outline can be modified.
     * To function correctly the context must be setup with a stencil buffer
     * and the stencil buffer must be cleared to zero before each render.
     *
     * osg::DisplaySettings::instance()->setMinimumNumStencilBits(1);
     * camera->setClearMask(clearMask | GL_STENCIL_BUFFER_BIT);
     * camera->setClearStencil(0);
     */
    class OSGFX_EXPORT Outline : public Effect
    {
    public:
        /// Constructor.
        Outline();

        /// Copy constructor.
        Outline(const Outline& copy, const osg::CopyOp& op = osg::CopyOp::SHALLOW_COPY) : Effect(copy, op) {
            _width = copy._width;
            _color = copy._color;
            _technique = copy._technique;
        }

        // Effect class info
        META_Effect(osgFX, Outline, "Outline",
                    "Stencil buffer based object outline effect.\n"
                    "This effect needs a properly setup stencil buffer.",
                    "Ulrich Hertlein");

        /// Set outline width.
        void setWidth(float w);

        /// Get outline width.
        float getWidth() const {
            return _width;
        }

        /// Set outline color.
        void setColor(const osg::Vec4& color);

        /// Get outline color.
        const osg::Vec4& getColor() const {
            return _color;
        }

    protected:
        /// Destructor.
        virtual ~Outline() {
        }

        /// Define available techniques.
        bool define_techniques();

    private:
        /// Outline width.
        float _width;

        /// Outline color.
        osg::Vec4 _color;

        /// Technique.
        class OutlineTechnique;
        OutlineTechnique* _technique;
    };

}

#endif

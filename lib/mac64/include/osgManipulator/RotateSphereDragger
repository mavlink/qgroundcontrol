/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
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
//osgManipulator - Copyright (C) 2007 Fugro-Jason B.V.

#ifndef OSGMANIPULATOR_ROTATESPHEREDRAGGER
#define OSGMANIPULATOR_ROTATESPHEREDRAGGER 1

#include <osgManipulator/Dragger>
#include <osgManipulator/Projector>

namespace osgManipulator {

/**
 * Dragger for performing 3D rotation on a sphere.
 */
class OSGMANIPULATOR_EXPORT RotateSphereDragger : public Dragger
{
    public:

        RotateSphereDragger();

        META_OSGMANIPULATOR_Object(osgManipulator,RotateSphereDragger)

        /**
         * Handle pick events on dragger and generate TranslateInLine commands.
         */
        virtual bool handle(const PointerInfo&, const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

        /** Setup default geometry for dragger. */
        void setupDefaultGeometry();

        /** Set/Get color for dragger. */
        inline void setColor(const osg::Vec4& color) { _color = color; setMaterialColor(_color,*this); }
        inline const osg::Vec4& getColor() const { return _color; }

        /**
         * Set/Get pick color for dragger. Pick color is color of the dragger
         * when picked. It gives a visual feedback to show that the dragger has
         * been picked.
         */
        inline void setPickColor(const osg::Vec4& color) { _pickColor = color; }
        inline const osg::Vec4& getPickColor() const { return _pickColor; }

    protected:

        virtual ~RotateSphereDragger();

        osg::ref_ptr<SpherePlaneProjector> _projector;

        osg::Vec3d  _prevWorldProjPt;
        bool        _prevPtOnSphere;
        osg::Matrix _startLocalToWorld, _startWorldToLocal;
        osg::Quat   _prevRotation;

        osg::Vec4 _color;
        osg::Vec4 _pickColor;
};


}

#endif

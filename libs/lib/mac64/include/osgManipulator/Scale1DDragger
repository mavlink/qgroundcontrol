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

#ifndef OSGMANIPULATOR_SCALE1DDRAGGER
#define OSGMANIPULATOR_SCALE1DDRAGGER 1

#include <osgManipulator/Dragger>
#include <osgManipulator/Projector>

namespace osgManipulator {

/**
 * Dragger for performing 1D scaling.
 */
class OSGMANIPULATOR_EXPORT Scale1DDragger : public Dragger
{
    public:

        enum ScaleMode
        {
            SCALE_WITH_ORIGIN_AS_PIVOT = 0,
            SCALE_WITH_OPPOSITE_HANDLE_AS_PIVOT
        };

        Scale1DDragger(ScaleMode scaleMode=SCALE_WITH_ORIGIN_AS_PIVOT);

        META_OSGMANIPULATOR_Object(osgManipulator,Scale1DDragger)

        /**
         * Handle pick events on dragger and generate TranslateInLine commands.
         */
        virtual bool handle(const PointerInfo& pi, const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

        /** Setup default geometry for dragger. */
        void setupDefaultGeometry();

        /** Set/Get min scale for dragger. */
        inline void  setMinScale(double min) { _minScale = min; }
        inline double getMinScale() const    { return _minScale; }

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

        /** Set/Get left and right handle nodes for dragger. */
        inline void setLeftHandleNode (osg::Node& node) { _leftHandleNode = &node; }
        inline void setRightHandleNode(osg::Node& node) { _rightHandleNode = &node; }
        inline osg::Node* getLeftHandleNode()  { return _leftHandleNode.get(); }
        inline const osg::Node* getLeftHandleNode() const  { return _leftHandleNode.get(); }
        inline osg::Node* getRightHandleNode() { return _rightHandleNode.get(); }
        inline const osg::Node* getRightHandleNode() const { return _rightHandleNode.get(); }

        /** Set left/right handle position. */
        inline void  setLeftHandlePosition(double pos)  { _projector->getLineStart() = osg::Vec3d(pos,0.0,0.0); }
        inline double getLeftHandlePosition() const     { return _projector->getLineStart()[0]; }
        inline void  setRightHandlePosition(double pos) { _projector->getLineEnd() = osg::Vec3d(pos,0.0,0.0); }
        inline double getRightHandlePosition() const    { return _projector->getLineEnd()[0]; }

    protected:

        virtual ~Scale1DDragger();

        osg::ref_ptr< LineProjector >   _projector;
        osg::Vec3d                      _startProjectedPoint;
        double                          _scaleCenter;
        double                          _minScale;

        osg::ref_ptr< osg::Node >       _leftHandleNode;
        osg::ref_ptr< osg::Node >       _rightHandleNode;

        osg::Vec4                       _color;
        osg::Vec4                       _pickColor;

        ScaleMode                       _scaleMode;
};


}

#endif

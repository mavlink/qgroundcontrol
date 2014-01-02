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

#ifndef OSGMANIPULATOR_TABBOXDRAGGER
#define OSGMANIPULATOR_TABBOXDRAGGER 1

#include <osgManipulator/TabPlaneDragger>

namespace osgManipulator {

/**
 * TabBoxDragger consists of 6 TabPlaneDraggers to form a box dragger that
 * performs translation and scaling.
 */
class OSGMANIPULATOR_EXPORT TabBoxDragger : public CompositeDragger
{
    public:

        TabBoxDragger();

        META_OSGMANIPULATOR_Object(osgManipulator,TabBoxDragger)

        /** Setup default geometry for dragger. */
        void setupDefaultGeometry();

        void setPlaneColor(const osg::Vec4& color);

    protected:

        virtual ~TabBoxDragger();

        std::vector< osg::ref_ptr< TabPlaneDragger > > _planeDraggers;
};


}

#endif

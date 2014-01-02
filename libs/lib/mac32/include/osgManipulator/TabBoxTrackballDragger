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

#ifndef OSGMANIPULATOR_TABBOXTRACKBALLDRAGGER
#define OSGMANIPULATOR_TABBOXTRACKBALLDRAGGER 1

#include <osgManipulator/TrackballDragger>
#include <osgManipulator/TabBoxDragger>

namespace osgManipulator {

/**
 * Dragger for performing rotation in all axes.
 */
class OSGMANIPULATOR_EXPORT TabBoxTrackballDragger : public CompositeDragger
{
    public:

        TabBoxTrackballDragger();

        META_OSGMANIPULATOR_Object(osgManipulator,TabBoxTrackballDragger)

        /** Setup default geometry for dragger. */
        void setupDefaultGeometry();

    protected:

        virtual ~TabBoxTrackballDragger();

        osg::ref_ptr<TrackballDragger>  _trackballDragger;
        osg::ref_ptr<TabBoxDragger>     _tabBoxDragger;
};


}

#endif

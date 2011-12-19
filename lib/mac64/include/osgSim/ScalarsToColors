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

#ifndef OSGSIM_SCALARSTCOLORS
#define OSGSIM_SCALARSTCOLORS 1

#include <osgSim/Export>

#include <osg/Vec4>
#include <osg/Referenced>

namespace osgSim
{
/**
ScalarsToColors defines the interface to map a scalar value to a color,
and provides a default implementation of the mapping functionaltity,
with colors ranging from black to white across the min - max scalar
range.
*/
class OSGSIM_EXPORT ScalarsToColors: public osg::Referenced
{
public:

    ScalarsToColors(float scalarMin, float scalarMax);
    virtual ~ScalarsToColors() {}

    /** Get the color for a given scalar value. */
    virtual osg::Vec4 getColor(float scalar) const;

    /** Get the minimum scalar value. */
    float getMin() const;

    /** Get the maximum scalar value. */
    float getMax() const;

private:

    float _min, _max;
};

}

#endif

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

#ifndef OSGUTIL_PLANEINTERSECTOR
#define OSGUTIL_PLANEINTERSECTOR 1

#include <osgUtil/IntersectionVisitor>

#include <osg/CoordinateSystemNode>

namespace osgUtil
{

/** Concrete class for implementing polytope intersections with the scene graph.
  * To be used in conjunction with IntersectionVisitor. */
class OSGUTIL_EXPORT PlaneIntersector : public Intersector
{
    public:

        /** Construct a PolytopeIntersector using speified polytope in MODEL coordinates.*/
        PlaneIntersector(const osg::Plane& plane, const osg::Polytope& boundingPolytope=osg::Polytope());

        /** Construct a PolytopeIntersector using speified polytope in specified coordinate frame.*/
        PlaneIntersector(CoordinateFrame cf, const osg::Plane& plane, const osg::Polytope& boundingPolytope=osg::Polytope());

        struct Intersection
        {
            Intersection() {}

            bool operator < (const Intersection& rhs) const
            {
                if (polyline < rhs.polyline) return true;
                if (rhs.polyline < polyline) return false;

                if (nodePath < rhs.nodePath) return true;
                if (rhs.nodePath < nodePath ) return false;

                return (drawable < rhs.drawable);
            }

            typedef std::vector<osg::Vec3d> Polyline;
            typedef std::vector<double> Attributes;

            osg::NodePath                   nodePath;
            osg::ref_ptr<osg::RefMatrix>    matrix;
            osg::ref_ptr<osg::Drawable>     drawable;
            Polyline                        polyline;
            Attributes                      attributes;

        };

        typedef std::vector<Intersection> Intersections;

        inline void insertIntersection(const Intersection& intersection) { getIntersections().push_back(intersection); }

        inline Intersections& getIntersections() { return _parent ? _parent->_intersections : _intersections; }


        void setRecordHeightsAsAttributes(bool flag) { _recordHeightsAsAttributes = flag; }

        bool getRecordHeightsAsAttributes() const { return _recordHeightsAsAttributes; }

        void setEllipsoidModel(osg::EllipsoidModel* em) { _em = em; }

        const osg::EllipsoidModel* getEllipsoidModel() const { return _em.get(); }

    public:

        virtual Intersector* clone(osgUtil::IntersectionVisitor& iv);

        virtual bool enter(const osg::Node& node);

        virtual void leave();

        virtual void intersect(osgUtil::IntersectionVisitor& iv, osg::Drawable* drawable);

        virtual void reset();

        virtual bool containsIntersections() { return !getIntersections().empty(); }

    protected:

        PlaneIntersector*                   _parent;

        bool                                _recordHeightsAsAttributes;
        osg::ref_ptr<osg::EllipsoidModel>   _em;

        osg::Plane                          _plane;
        osg::Polytope                       _polytope;

        Intersections                       _intersections;

};

}

#endif


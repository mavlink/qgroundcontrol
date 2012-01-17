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

#ifndef OSG_OBSERVERNODEPATH
#define OSG_OBSERVERNODEPATH 1

#include <osg/Node>
#include <osg/observer_ptr>
#include <vector>

namespace osg {

typedef std::vector< osg::ref_ptr<osg::Node> > RefNodePath;

/** ObserverNodePath is an observer class for tracking changes to a NodePath,
  * that automatically invalidates it when nodes are deleted.*/
class OSG_EXPORT ObserverNodePath
{
    public:
        ObserverNodePath();

        ObserverNodePath(const ObserverNodePath& rhs);

        ObserverNodePath(const osg::NodePath& nodePath);

        ~ObserverNodePath();

        ObserverNodePath& operator = (const ObserverNodePath& rhs);

        /** get the NodePath from the first parental chain back to root, plus the specified node.*/
        void setNodePathTo(osg::Node* node);

        void setNodePath(const osg::RefNodePath& nodePath);

        void setNodePath(const osg::NodePath& nodePath);

        void clearNodePath();

        /** Get a thread safe RefNodePath, return true if NodePath is valid.*/
        bool getRefNodePath(RefNodePath& refNodePath) const;

        /** Get a lightweight NodePath that isn't thread safe but
          * may be safely used in single threaded applications, or when
          * its known that the NodePath won't be invalidated during usage
          * of the NodePath. return true if NodePath is valid.*/
        bool getNodePath(NodePath& nodePath) const;

        bool empty() const
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
            return _nodePath.empty();
        }

    protected:

        void _setNodePath(const osg::NodePath& nodePath);
        void _clearNodePath();

        typedef std::vector< osg::observer_ptr<osg::Node> > ObsNodePath;
        mutable OpenThreads::Mutex      _mutex;
        ObsNodePath                     _nodePath;
};

}

#endif

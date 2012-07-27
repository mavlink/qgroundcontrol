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

#ifndef OSGMANIPULATOR_COMMANDMANAGER
#define OSGMANIPULATOR_COMMANDMANAGER 1

#include <osgManipulator/Dragger>
#include <osgManipulator/Selection>
#include <osgManipulator/Constraint>

namespace osgManipulator {

/**
 * Deprecated.
 * CommandManager class is now no longer required as Dragger now matains all references to Constraints and Selections (now just generic MatrixTransforms).
 * To replace CommandManager usage simple replace cmdMgr->connect(*dragger, *selection) with dragger->addTransformUpdating(selection) and
 * cmdMgr->connect(*dragger, *selection) with dragger->addConstaint(constraint).
 */
class CommandManager : public osg::Referenced
{
    public:
        CommandManager() {}

        bool connect(Dragger& dragger, Selection& selection)
        {
            dragger.addTransformUpdating(&selection);

            return true;
        }

        bool connect(Dragger& dragger, Constraint& constraint)
        {
            dragger.addConstraint(&constraint);

            return true;
        }

        bool disconnect(Dragger& dragger)
        {
            dragger.getConstraints().clear();
            dragger.getDraggerCallbacks().clear();

            return true;
        }

        typedef std::list< osg::ref_ptr<Selection> > Selections;

        Selections getConnectedSelections(Dragger& dragger)
        {
            Selections selections;

            for(Dragger::DraggerCallbacks::iterator itr = dragger.getDraggerCallbacks().begin();
                itr != dragger.getDraggerCallbacks().end();
                ++itr)
            {
                DraggerCallback* dc = itr->get();
                DraggerTransformCallback* dtc = dynamic_cast<DraggerTransformCallback*>(dc);
                if (dtc && dtc->getTransform()) selections.push_back(dtc->getTransform());
            }

            return selections;
        }

    protected:

        virtual ~CommandManager() {}

};

}

#endif

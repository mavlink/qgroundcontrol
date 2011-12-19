/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2009 Robert Osfield 
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

#ifndef OSGVIEWER_GRAPHICSHANDLECARBON
#define OSGVIEWER_GRAPHICSHANDLECARBON 1

#include <osgViewer/Export>

#include <Carbon/Carbon.h>
#include <AGL/agl.h>


namespace osgViewer
{

/** Class to encapsulate platform-specific OpenGL context handle variables.
  * Derived osg::GraphicsContext classes can inherit from this class to
  * share OpenGL resources.*/

class OSGVIEWER_EXPORT GraphicsHandleCarbon
{
    public:
    
        GraphicsHandleCarbon():
            _context(0) {}

        /** Set native AGL graphics context.*/        
        inline void setAGLContext(AGLContext context) { _context = context; }

        /** Get native AGL graphics context.*/        
        inline AGLContext getAGLContext() const { return _context; }

    protected:
        
        AGLContext      _context;
};

}

#endif

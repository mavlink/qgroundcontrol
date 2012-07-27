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

#ifndef OSGVIEWER_GRAPHICSHANDLEX11
#define OSGVIEWER_GRAPHICSHANDLEX11 1

#include <osgViewer/Export>


#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE)
    #define OSG_USE_EGL
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <EGL/egl.h>
#else
    #define GLX_GLXEXT_PROTOTYPES  1
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <GL/glx.h>
    #ifndef GLX_VERSION_1_3
        typedef XID GLXPbuffer;
    #endif
#endif

namespace osgViewer
{

/** Class to encapsulate platform-specific OpenGL context handle variables.
  * Derived osg::GraphicsContext classes can inherit from this class to
  * share OpenGL resources.*/

class OSGVIEWER_EXPORT GraphicsHandleX11
{
    public:
    
        GraphicsHandleX11():
            _display(0),
            _context(0) {}

        /** Set X11 display.*/        
        inline void setDisplay(Display* display) { _display = display; }

        /** Get X11 display.*/        
        inline Display* getDisplay() const { return _display; }

        #ifdef OSG_USE_EGL
            typedef EGLContext Context;
            typedef EGLSurface Pbuffer;
        #else
            typedef GLXContext Context;
            typedef GLXPbuffer Pbuffer;
        #endif

        /** Set native OpenGL graphics context.*/
        inline void setContext(Context context) { _context = context; }

        /** Get native OpenGL graphics context.*/        
        inline Context getContext() const { return _context; }

    protected:
        
        Display*        _display;
        Context         _context;
};

}

#endif

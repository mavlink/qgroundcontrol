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

#ifndef OSGMANIPULATOR_EXPORT_
#define OSGMANIPULATOR_EXPORT_ 1

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__) || defined( __MWERKS__)
    #  if defined( OSG_LIBRARY_STATIC )
    #    define OSGMANIPULATOR_EXPORT
    #  elif defined( OSGMANIPULATOR_LIBRARY )
    #    define OSGMANIPULATOR_EXPORT   __declspec(dllexport)
    #  else
    #    define OSGMANIPULATOR_EXPORT   __declspec(dllimport)
    #  endif
#else
    #  define OSGMANIPULATOR_EXPORT
#endif

#define META_OSGMANIPULATOR_Object(library,name) \
virtual osg::Object* cloneType() const { return new name (); } \
virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const name *>(obj)!=NULL; } \
virtual const char* libraryName() const { return #library; }\
virtual const char* className() const { return #name; }


/**

\namespace osgManipulator

The osgManipulator library is a NodeKit that extends the core scene graph to support 3D interactive manipulators.
*/

#endif

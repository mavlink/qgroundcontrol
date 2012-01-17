/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2010 Robert Osfield
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

#ifndef OSGGA_TERRAIN_MANIPULATOR
#define OSGGA_TERRAIN_MANIPULATOR 1

#include <osgGA/OrbitManipulator>


namespace osgGA {


class OSGGA_EXPORT TerrainManipulator : public OrbitManipulator
{
   typedef OrbitManipulator inherited;

public:

   TerrainManipulator( int flags = DEFAULT_SETTINGS );
   TerrainManipulator( const TerrainManipulator& tm,
                       const osg::CopyOp& copyOp = osg::CopyOp::SHALLOW_COPY );

   META_Object( osgGA, TerrainManipulator );

   enum RotationMode
   {
      ELEVATION_AZIM_ROLL,
      ELEVATION_AZIM
   };

   virtual void setRotationMode(RotationMode mode);
   RotationMode getRotationMode() const;

   virtual void setByMatrix( const osg::Matrixd& matrix );

   virtual void setTransformation( const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up );

   virtual void setNode( osg::Node* node );

protected:

   virtual bool performMovementMiddleMouseButton( const double eventTimeDelta, const double dx, const double dy );
   virtual bool performMovementRightMouseButton( const double eventTimeDelta, const double dx, const double dy );

   bool intersect( const osg::Vec3d& start, const osg::Vec3d& end, osg::Vec3d& intersection ) const;
   void clampOrientation();

   osg::Vec3d _previousUp;
};


}

#endif /* OSGGA_TERRAIN_MANIPULATOR */

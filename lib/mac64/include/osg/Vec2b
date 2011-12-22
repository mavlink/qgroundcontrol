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

#ifndef OSG_VEC2B
#define OSG_VEC2B 1

namespace osg {

/** General purpose float triple.
  * Uses include representation of color coordinates.
  * No support yet added for float * Vec2b - is it necessary?
  * Need to define a non-member non-friend operator*  etc.
  * Vec2b * float is okay
*/
class Vec2b
{
    public:

        // Methods are defined here so that they are implicitly inlined

        /** Data type of vector components.*/
        typedef signed char value_type;

        /** Number of vector components. */
        enum { num_components = 2 };
        
        /** Vec member variable. */
        value_type _v[2];

        /** Constructor that sets all components of the vector to zero */
        Vec2b() { _v[0]=0; _v[1]=0; }
        
        Vec2b(value_type r, value_type g)
        {
            _v[0]=r; _v[1]=g;
        }

        inline bool operator == (const Vec2b& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1]; }

        inline bool operator != (const Vec2b& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1]; }

        inline bool operator <  (const Vec2b& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else return (_v[1]<v._v[1]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y)
        {
            _v[0]=x; _v[1]=y;
        }

        inline void set( const Vec2b& rhs)
        {
            _v[0]=rhs._v[0]; _v[1]=rhs._v[1];
        }

        inline value_type& operator [] (int i) { return _v[i]; }
        inline value_type operator [] (int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }

        inline value_type& r() { return _v[0]; }
        inline value_type& g() { return _v[1]; }

        inline value_type r() const { return _v[0]; }
        inline value_type g() const { return _v[1]; }

        /** Multiply by scalar. */
        inline Vec2b operator * (float rhs) const
        {
            Vec2b col(*this);
            col *= rhs;
            return col;
        }

        /** Unary multiply by scalar. */
        inline Vec2b& operator *= (float rhs)
        {
            _v[0]=(value_type)((float)_v[0]*rhs);
            _v[1]=(value_type)((float)_v[1]*rhs);            
            return *this;
        }

        /** Divide by scalar. */
        inline Vec2b operator / (float rhs) const
        {
            Vec2b col(*this);
            col /= rhs;
            return col;
        }

        /** Unary divide by scalar. */
        inline Vec2b& operator /= (float rhs)
        {
            float div = 1.0f/rhs;
            *this *= div;
            return *this;
        }

        /** Binary vector add. */
        inline Vec2b operator + (const Vec2b& rhs) const
        {
            return Vec2b(_v[0]+rhs._v[0], _v[1]+rhs._v[1]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec2b& operator += (const Vec2b& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];            
            return *this;
        }

        /** Binary vector subtract. */
        inline Vec2b operator - (const Vec2b& rhs) const
        {
            return Vec2b(_v[0]-rhs._v[0], _v[1]-rhs._v[1]);
        }

        /** Unary vector subtract. */
        inline Vec2b& operator -= (const Vec2b& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];            
            return *this;
        }

};    // end of class Vec2b



}    // end of namespace osg

#endif

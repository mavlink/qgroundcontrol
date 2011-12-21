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

#ifndef OSG_VEC4S
#define OSG_VEC4S 1

namespace osg {

/** General purpose float quad. Uses include representation
  * of color coordinates.
  * No support yet added for float * Vec4f - is it necessary?
  * Need to define a non-member non-friend operator*  etc.
  *    Vec4f * float is okay
*/
class Vec4s
{
    public:

        /** Data type of vector components.*/
        typedef short value_type;

        /** Number of vector components. */
        enum { num_components = 4 };
        
        /** Vec member variable. */
        value_type _v[4];

        /** Constructor that sets all components of the vector to zero */
        Vec4s() { _v[0]=0; _v[1]=0; _v[2]=0; _v[3]=0; }

        Vec4s(value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x;
            _v[1]=y;
            _v[2]=z;
            _v[3]=w;
        }

        inline bool operator == (const Vec4s& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2] && _v[3]==v._v[3]; }
        inline bool operator != (const Vec4s& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2] || _v[3]!=v._v[3]; }
        inline bool operator <  (const Vec4s& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else if (_v[2]<v._v[2]) return true;
            else if (_v[2]>v._v[2]) return false;
            else return (_v[3]<v._v[3]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set( value_type x, value_type y, value_type z, value_type w)
        {
            _v[0]=x; _v[1]=y; _v[2]=z; _v[3]=w;
        }

        inline value_type& operator [] (unsigned int i) { return _v[i]; }
        inline value_type  operator [] (unsigned int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }
        inline value_type& z() { return _v[2]; }
        inline value_type& w() { return _v[3]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }
        inline value_type w() const { return _v[3]; }

        inline value_type& r() { return _v[0]; }
        inline value_type& g() { return _v[1]; }
        inline value_type& b() { return _v[2]; }
        inline value_type& a() { return _v[3]; }

        inline value_type r() const { return _v[0]; }
        inline value_type g() const { return _v[1]; }
        inline value_type b() const { return _v[2]; }
        inline value_type a() const { return _v[3]; }

        /** Multiply by scalar. */
        inline Vec4s operator * (value_type rhs) const
        {
            return Vec4s(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs, _v[3]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec4s& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            _v[3]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline Vec4s operator / (value_type rhs) const
        {
            return Vec4s(_v[0]/rhs, _v[1]/rhs, _v[2]/rhs, _v[3]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec4s& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            _v[2]/=rhs;
            _v[3]/=rhs;
            return *this;
        }


        /** Binary vector multiply. */
        inline Vec4s operator * (const Vec4s& rhs) const
        {
            return Vec4s(_v[0]*rhs._v[0], _v[1]*rhs._v[1],
                         _v[2]*rhs._v[2], _v[3]*rhs._v[3]);
        }

        /** Binary vector add. */
        inline Vec4s operator + (const Vec4s& rhs) const
        {
            return Vec4s(_v[0]+rhs._v[0], _v[1]+rhs._v[1],
                         _v[2]+rhs._v[2], _v[3]+rhs._v[3]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec4s& operator += (const Vec4s& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            _v[3] += rhs._v[3];
            return *this;
        }

        /** Binary vector subtract. */
        inline Vec4s operator - (const Vec4s& rhs) const
        {
            return Vec4s(_v[0]-rhs._v[0], _v[1]-rhs._v[1],
                         _v[2]-rhs._v[2], _v[3]-rhs._v[3] );
        }

        /** Unary vector subtract. */
        inline Vec4s& operator -= (const Vec4s& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            _v[3]-=rhs._v[3];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec4s. */
        inline Vec4s operator - () const
        {
            return Vec4s (-_v[0], -_v[1], -_v[2], -_v[3]);
        }

};    // end of class Vec4s


/** multiply by vector components. */
inline Vec4s componentMultiply(const Vec4s& lhs, const Vec4s& rhs)
{
    return Vec4s(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2], lhs[3]*rhs[3]);
}

/** divide rhs components by rhs vector components. */
inline Vec4s componentDivide(const Vec4s& lhs, const Vec4s& rhs)
{
    return Vec4s(lhs[0]/rhs[0], lhs[1]/rhs[1], lhs[2]/rhs[2], lhs[3]/rhs[3]);
}

}    // end of namespace osg

#endif


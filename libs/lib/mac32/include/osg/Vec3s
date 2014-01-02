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

#ifndef OSG_VEC3S
#define OSG_VEC3S 1

namespace osg {

class Vec3s
{
    public:

        /** Data type of vector components.*/
        typedef short value_type;

        /** Number of vector components. */
        enum { num_components = 3 };
        
        value_type _v[3];

        /** Constructor that sets all components of the vector to zero */
        Vec3s() { _v[0]=0; _v[1]=0; _v[2]=0; }
        
        Vec3s(value_type r, value_type g, value_type b) { _v[0]=r; _v[1]=g; _v[2]=b;  }

        inline bool operator == (const Vec3s& v) const { return _v[0]==v._v[0] && _v[1]==v._v[1] && _v[2]==v._v[2]; }
        inline bool operator != (const Vec3s& v) const { return _v[0]!=v._v[0] || _v[1]!=v._v[1] || _v[2]!=v._v[2]; }
        inline bool operator <  (const Vec3s& v) const
        {
            if (_v[0]<v._v[0]) return true;
            else if (_v[0]>v._v[0]) return false;
            else if (_v[1]<v._v[1]) return true;
            else if (_v[1]>v._v[1]) return false;
            else return (_v[2]<v._v[2]);
        }

        inline value_type* ptr() { return _v; }
        inline const value_type* ptr() const { return _v; }

        inline void set(value_type r, value_type g, value_type b)
        {
            _v[0]=r; _v[1]=g; _v[2]=b;
        }

        inline void set( const Vec3s& rhs)
        {
            _v[0]=rhs._v[0]; _v[1]=rhs._v[1]; _v[2]=rhs._v[2];
        }

        inline value_type& operator [] (unsigned int i) { return _v[i]; }
        inline value_type operator [] (unsigned int i) const { return _v[i]; }

        inline value_type& x() { return _v[0]; }
        inline value_type& y() { return _v[1]; }
        inline value_type& z() { return _v[2]; }

        inline value_type x() const { return _v[0]; }
        inline value_type y() const { return _v[1]; }
        inline value_type z() const { return _v[2]; }

        inline value_type& r() { return _v[0]; }
        inline value_type& g() { return _v[1]; }
        inline value_type& b() { return _v[2]; }

        inline value_type r() const { return _v[0]; }
        inline value_type g() const { return _v[1]; }
        inline value_type b() const { return _v[2]; }

        /** Multiply by scalar. */
        inline Vec3s operator * (value_type rhs) const
        {
            return Vec3s(_v[0]*rhs, _v[1]*rhs, _v[2]*rhs);
        }

        /** Unary multiply by scalar. */
        inline Vec3s& operator *= (value_type rhs)
        {
            _v[0]*=rhs;
            _v[1]*=rhs;
            _v[2]*=rhs;
            return *this;
        }

        /** Divide by scalar. */
        inline Vec3s operator / (value_type rhs) const
        {
            return Vec3s(_v[0]/rhs, _v[1]/rhs, _v[2]/rhs);
        }

        /** Unary divide by scalar. */
        inline Vec3s& operator /= (value_type rhs)
        {
            _v[0]/=rhs;
            _v[1]/=rhs;
            _v[2]/=rhs;
            return *this;
        }

        /** Binary vector multiply. */
        inline Vec3s operator * (const Vec3s& rhs) const
        {
            return Vec3s(_v[0]*rhs._v[0], _v[1]*rhs._v[1], _v[2]*rhs._v[2]);
        }

        /** Binary vector add. */
        inline Vec3s operator + (const Vec3s& rhs) const
        {
            return Vec3s(_v[0]+rhs._v[0], _v[1]+rhs._v[1], _v[2]+rhs._v[2]);
        }

        /** Unary vector add. Slightly more efficient because no temporary
          * intermediate object.
        */
        inline Vec3s& operator += (const Vec3s& rhs)
        {
            _v[0] += rhs._v[0];
            _v[1] += rhs._v[1];
            _v[2] += rhs._v[2];
            return *this;
        }

        /** Binary vector subtract. */
        inline Vec3s operator - (const Vec3s& rhs) const
        {
            return Vec3s(_v[0]-rhs._v[0], _v[1]-rhs._v[1], _v[2]-rhs._v[2]);
        }
        
        /** Unary vector subtract. */
        inline Vec3s& operator -= (const Vec3s& rhs)
        {
            _v[0]-=rhs._v[0];
            _v[1]-=rhs._v[1];
            _v[2]-=rhs._v[2];
            return *this;
        }

        /** Negation operator. Returns the negative of the Vec3s. */
        inline Vec3s operator - () const
        {
            return Vec3s (-_v[0], -_v[1], -_v[2]);
        }

};    // end of class Vec3s


/** multiply by vector components. */
inline Vec3s componentMultiply(const Vec3s& lhs, const Vec3s& rhs)
{
    return Vec3s(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2]);
}

/** divide rhs components by rhs vector components. */
inline Vec3s componentDivide(const Vec3s& lhs, const Vec3s& rhs)
{
    return Vec3s(lhs[0]/rhs[0], lhs[1]/rhs[1], lhs[2]/rhs[2]);
}

}    // end of namespace osg

#endif


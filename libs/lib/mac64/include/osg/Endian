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

#ifndef OSG_ENDIAN
#define OSG_ENDIAN 1

#include <algorithm>

namespace osg {

enum Endian
{
    BigEndian,
    LittleEndian
};

inline Endian getCpuByteOrder()
{
    union {
        char big_endian_1[2];
        short is_it_really_1;
    } u;
    u.big_endian_1[0] = 0;
    u.big_endian_1[1] = 1;

    if (u.is_it_really_1 == 1)
        return BigEndian;
    else 
        return LittleEndian;
} 

inline void swapBytes( char* in, unsigned int size )
{
    char* start = in;
    char* end = start+size-1;
    while (start<end)
    {
        std::swap(*start++,*end--);
    }
}

inline void swapBytes2( char* in )
{
    std::swap(in[0],in[1]);
}

inline void swapBytes4( char* in )
{
    std::swap(in[0],in[3]);
    std::swap(in[1],in[2]);
}

inline void swapBytes8( char* in )
{
    std::swap(in[0],in[7]);
    std::swap(in[1],in[6]);
    std::swap(in[2],in[5]);
    std::swap(in[3],in[4]);
}

inline void swapBytes16( char* in )
{
    std::swap(in[0],in[15]);
    std::swap(in[1],in[14]);
    std::swap(in[2],in[13]);
    std::swap(in[3],in[12]);
    std::swap(in[4],in[11]);
    std::swap(in[5],in[10]);
    std::swap(in[6],in[9]);
    std::swap(in[7],in[8]);
}

}

#endif

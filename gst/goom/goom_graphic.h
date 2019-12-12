/* Goom Project
 * Copyright (C) <2003> iOS-Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef GRAPHIC_H
#define GRAPHIC_H

typedef unsigned int Uint;

typedef struct
{
  unsigned short r, v, b;
}
Color;

extern const Color BLACK;
extern const Color WHITE;
extern const Color RED;
extern const Color BLUE;
extern const Color GREEN;
extern const Color YELLOW;
extern const Color ORANGE;
extern const Color VIOLET;


#ifdef COLOR_BGRA

#define B_CHANNEL 0xFF000000
#define G_CHANNEL 0x00FF0000
#define R_CHANNEL 0x0000FF00
#define A_CHANNEL 0x000000FF
#define B_OFFSET  24
#define G_OFFSET  16
#define R_OFFSET  8
#define A_OFFSET  0

typedef union _PIXEL {
  struct {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
  } channels;
  unsigned int val;
  unsigned char cop[4];
} Pixel;

#else

#define A_CHANNEL 0xFF000000
#define R_CHANNEL 0x00FF0000
#define G_CHANNEL 0x0000FF00
#define B_CHANNEL 0x000000FF
#define A_OFFSET  24
#define R_OFFSET  16
#define G_OFFSET  8
#define B_OFFSET  0

typedef union _PIXEL {
  struct {
    unsigned char a;
    unsigned char r;
    unsigned char g;
    unsigned char b;
  } channels;
  unsigned int val;
  unsigned char cop[4];
} Pixel;

#endif /* COLOR_BGRA */

/*
inline void setPixelRGB (Pixel * buffer, Uint x, Uint y, Color c);
inline void getPixelRGB (Pixel * buffer, Uint x, Uint y, Color * c);
*/


#endif /* GRAPHIC_H */

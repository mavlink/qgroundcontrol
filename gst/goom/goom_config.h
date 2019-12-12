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
#include <glib.h>

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define COLOR_ARGB
#else
#define COLOR_BGRA
#endif

#if 1
/* ndef COLOR_BGRA */
/* position des composantes */
    #define BLEU 0
    #define VERT 1
    #define ROUGE 2
    #define ALPHA 3
#else
    #define ROUGE 1
    #define BLEU 3
    #define VERT 2
    #define ALPHA 0
#endif

#if defined (BUILD_MMX) && defined (HAVE_GCC_ASM)

#define HAVE_MMX
#endif


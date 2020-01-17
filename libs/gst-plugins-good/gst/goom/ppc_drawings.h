/* Goom Project
 * Copyright (C) <2003> Guillaume Borios, iOS-Software
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

/* Generic PowerPC Code */
void ppc_brightness_generic(Pixel *src, Pixel *dest, int size, int coeff);

/* G4 Specific PowerPC Code (Possible use of Altivec and Data Streams) */
void ppc_brightness_G4(Pixel *src, Pixel *dest, int size, int coeff);

/* G5 Specific PowerPC Code (Possible use of Altivec) */
void ppc_brightness_G5(Pixel *src, Pixel *dest, int size, int coeff);


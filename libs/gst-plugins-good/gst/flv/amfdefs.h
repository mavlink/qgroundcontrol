/* GStreamer
 *
 * Copyright (c) 2011 Jan Schmidt <thaytan@noraisin.net>
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

#ifndef __AMFDEFS_H__
#define __AMFDEFS_H__

#include <glib.h>

#define AMF0_NUMBER_MARKER 0x0
#define AMF0_BOOLEAN_MARKER 0x1
#define AMF0_STRING_MARKER 0x2
#define AMF0_OBJECT_MARKER 0x3
#define AMF0_MOVIECLIP_MARKER 0x4 /* Reserved, not supported */
#define AMF0_NULL_MARKER 0x5
#define AMF0_UNDEFINED_MARKER 0x6
#define AMF0_REFERENCE_MARKER 0x7
#define AMF0_ECMA_ARRAY_MARKER 0x8
#define AMF0_OBJECT_END_MARKER 0x9
#define AMF0_STRICT_ARRAY_MARKER 0xA
#define AMF0_DATE_MARKER 0xB
#define AMF0_LONG_STRING_MARKER 0xC
#define AMF0_UNSUPPORTED_MARKER 0xD
#define AMF0_RECORDSET_MARKER 0xE /* Reserved, not supported */
#define AMF0_XML_DOCUMENT_MARKER 0xF
#define AMF0_TYPED_OBJECT_MARKER 0x10

#endif

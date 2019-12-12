/* GStreamer ReplayGain plugin
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 *
 * replaygain.h: Plugin providing ReplayGain related elements
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __REPLAYGAIN_H__
#define __REPLAYGAIN_H__

G_BEGIN_DECLS

/* Reference level (in dBSPL).  The 2001 proposal specifies 83.  This was
 * changed later in all implementations to 89, which is the new, official value:
 * David Robinson acknowledged the change but didn't update the website yet. */

#define RG_REFERENCE_LEVEL 89.

G_END_DECLS

#endif /* __REPLAYGAIN_H__ */

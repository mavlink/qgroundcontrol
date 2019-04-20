/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_bin.h,v 1.9 2006/01/22 13:23:37 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_BIN_H
#define _LV_BIN_H

#include <libvisual/lv_actor.h>
#include <libvisual/lv_input.h>
#include <libvisual/lv_morph.h>
#include <libvisual/lv_video.h>
#include <libvisual/lv_time.h>

VISUAL_BEGIN_DECLS

#define VISUAL_BIN(obj)					(VISUAL_CHECK_CAST ((obj), VisBin))

typedef enum {
	VISUAL_SWITCH_STYLE_DIRECT,
	VISUAL_SWITCH_STYLE_MORPH
} VisBinSwitchStyle;

typedef enum {
	VISUAL_BIN_DEPTH_LOWEST,
	VISUAL_BIN_DEPTH_HIGHEST
} VisBinDepth;

typedef struct _VisBin VisBin;

struct _VisBin {
	VisObject	 object;

	int		 managed;
	VisActor	*actor;
	VisVideo	*actvideo;
	VisVideo	*privvid;
	
	int		 actmorphmanaged;
	VisVideo	*actmorphvideo;
	VisActor	*actmorph;

	int		 inputmanaged;
	VisInput	*input;

	int		 morphmanaged;
	VisMorph	*morph;
	int		 morphstyle;
	int		 morphing;
	int		 morphautomatic;
	int		 morphsteps;
	int		 morphstepsdone;
	float		 morphrate;
	VisMorphMode	 morphmode;
	VisTime		 morphtime;

	int		 depthpreferred;	/* Prefered depth, highest or lowest */
	int		 depthflag;		/* Supported depths */
	int		 depthold;		/* Previous depth */
	int		 depth;			/* Depth we're running in */
	int		 depthchanged;		/* Set TRUE if the depth has changed */
	int		 depthfromGL;		/* Set when switching away from openGL */
	int		 depthforced;		/* Contains forced depth value, for the actmorph so we've got smooth transformations */
	int		 depthforcedmain;	/* Contains forced depth value, for the main actor */
};

/* prototypes */
VisBin *visual_bin_new (void);

int visual_bin_realize (VisBin *bin);

int visual_bin_set_actor (VisBin *bin, VisActor *actor);
VisActor *visual_bin_get_actor (VisBin *bin);

int visual_bin_set_input (VisBin *bin, VisInput *input);
VisInput *visual_bin_get_input (VisBin *bin);

int visual_bin_set_morph (VisBin *bin, VisMorph *morph);
int visual_bin_set_morph_by_name (VisBin *bin, char *morphname);
VisMorph *visual_bin_get_morph (VisBin *bin);

int visual_bin_connect (VisBin *bin, VisActor *actor, VisInput *input);
int visual_bin_connect_by_names (VisBin *bin, char *actname, char *inname);

int visual_bin_sync (VisBin *bin, int noevent);

int visual_bin_set_video (VisBin *bin, VisVideo *video);

int visual_bin_set_supported_depth (VisBin *bin, int depthflag);
int visual_bin_set_preferred_depth (VisBin *bin, VisBinDepth depthpreferred);
int visual_bin_set_depth (VisBin *bin, int depth);
int visual_bin_get_depth (VisBin *bin);
int visual_bin_depth_changed (VisBin *bin);

VisPalette *visual_bin_get_palette (VisBin *bin);

int visual_bin_switch_actor_by_name (VisBin *bin, char *actname);
int visual_bin_switch_actor (VisBin *bin, VisActor *actor);
int visual_bin_switch_finalize (VisBin *bin);
int visual_bin_switch_set_style (VisBin *bin, VisBinSwitchStyle style);
int visual_bin_switch_set_steps (VisBin *bin, int steps);
int visual_bin_switch_set_automatic (VisBin *bin, int automatic);
int visual_bin_switch_set_rate (VisBin *bin, float rate);
int visual_bin_switch_set_mode (VisBin *bin, VisMorphMode mode);
int visual_bin_switch_set_time (VisBin *bin, long sec, long usec);

int visual_bin_run (VisBin *bin);

VISUAL_END_DECLS

#endif /* _LV_BIN_H */

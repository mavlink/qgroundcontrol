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
#ifndef _GOOMCORE_H
#define _GOOMCORE_H

#include "goom_config.h"
#include "goom_plugin_info.h"

#define NB_FX 10

PluginInfo *goom_init (guint32 resx, guint32 resy);
void goom_set_resolution (PluginInfo *goomInfo, guint32 resx, guint32 resy);

/*
 * forceMode == 0 : do nothing
 * forceMode == -1 : lock the FX
 * forceMode == 1..NB_FX : force a switch to FX n# forceMode
 */
guint32 *goom_update (PluginInfo *goomInfo, gint16 data[2][512], int forceMode, float fps);

/* returns 0 if the buffer wasn't accepted */
int goom_set_screenbuffer(PluginInfo *goomInfo, void *buffer);

void goom_close (PluginInfo *goomInfo);

#endif

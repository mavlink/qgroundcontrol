/* Goom Project
 * Copyright (C) <2003> Jean-Christophe Hoelt <jeko@free.fr>
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
#ifndef _VISUAL_FX_H
#define _VISUAL_FX_H

#include "goom_config_param.h"
#include "goom_graphic.h"
#include "goom_typedefs.h"

struct _VISUAL_FX {
  void (*init) (struct _VISUAL_FX *_this, PluginInfo *info);
  void (*free) (struct _VISUAL_FX *_this);
  void (*apply) (struct _VISUAL_FX *_this, Pixel *src, Pixel *dest, PluginInfo *info);
  void *fx_data;

  PluginParameters *params;
};

#endif

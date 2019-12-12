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

#include "goom_config_param.h"
#include <string.h>

static void
empty_fct (PluginParam * dummy)
{
}

void
goom_secure_param (PluginParam * p)
{
  p->changed = empty_fct;
  p->change_listener = empty_fct;
  p->user_data = 0;
  p->name = p->desc = 0;
  p->rw = 1;
}

void
goom_secure_f_param (PluginParam * p, const char *name)
{
  secure_param (p);

  p->name = name;
  p->type = PARAM_FLOATVAL;
  FVAL (*p) = 0.5f;
  FMIN (*p) = 0.0f;
  FMAX (*p) = 1.0f;
  FSTEP (*p) = 0.01f;
}

void
goom_secure_f_feedback (PluginParam * p, const char *name)
{
  secure_f_param (p, name);

  p->rw = 0;
}

void
goom_secure_s_param (PluginParam * p, const char *name)
{
  secure_param (p);

  p->name = name;
  p->type = PARAM_STRVAL;
  SVAL (*p) = 0;
}

void
goom_secure_b_param (PluginParam * p, const char *name, int value)
{
  secure_param (p);

  p->name = name;
  p->type = PARAM_BOOLVAL;
  BVAL (*p) = value;
}

void
goom_secure_i_param (PluginParam * p, const char *name)
{
  secure_param (p);

  p->name = name;
  p->type = PARAM_INTVAL;
  IVAL (*p) = 50;
  IMIN (*p) = 0;
  IMAX (*p) = 100;
  ISTEP (*p) = 1;
}

void
goom_secure_i_feedback (PluginParam * p, const char *name)
{
  secure_i_param (p, name);

  p->rw = 0;
}

void
goom_plugin_parameters (PluginParameters * p, const char *name, int nb)
{
  p->name = name;
  p->desc = "";
  p->nbParams = nb;
  p->params = malloc (nb * sizeof (PluginParam *));
}

void
goom_plugin_parameters_free (PluginParameters * p)
{
  free (p->params);
}

/*---------------------------------------------------------------------------*/

void
goom_set_str_param_value (PluginParam * p, const char *str)
{
  int len = strlen (str);

  if (SVAL (*p))
    SVAL (*p) = (char *) realloc (SVAL (*p), len + 1);
  else
    SVAL (*p) = (char *) malloc (len + 1);
  memcpy (SVAL (*p), str, len + 1);
}

void
goom_set_list_param_value (PluginParam * p, const char *str)
{
  int len = strlen (str);

#ifdef VERBOSE
  printf ("%s: %d\n", str, len);
#endif
  if (LVAL (*p))
    LVAL (*p) = (char *) realloc (LVAL (*p), len + 1);
  else
    LVAL (*p) = (char *) malloc (len + 1);
  memcpy (LVAL (*p), str, len + 1);
}

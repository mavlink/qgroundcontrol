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
#ifndef _CONFIG_PARAM_H
#define _CONFIG_PARAM_H

#include <stdlib.h>

/*
 * File created on 2003-05-24 by Jeko.
 * (c)2003, JC Hoelt for iOS-software.
 *
 * LGPL Licence.
 */

typedef enum {
  PARAM_INTVAL,
  PARAM_FLOATVAL,
  PARAM_BOOLVAL,
  PARAM_STRVAL,
  PARAM_LISTVAL,
} ParamType;

struct IntVal {
  int value;
  int min;
  int max;
  int step;
};
struct FloatVal {
  float value;
  float min;
  float max;
  float step;
};
struct StrVal {
  char *value;
};
struct ListVal {
  char *value;
  int nbChoices;
  char **choices;
};
struct BoolVal {
  int value;
};


typedef struct _PARAM {
  const char *name;
  const char *desc;
  char rw;
  ParamType type;
  union {
    struct IntVal ival;
    struct FloatVal fval;
    struct StrVal sval;
    struct ListVal slist;
    struct BoolVal bval;
  } param;
  
  /* used by the core to inform the GUI of a change */
  void (*change_listener)(struct _PARAM *_this);

  /* used by the GUI to inform the core of a change */
  void (*changed)(struct _PARAM *_this);
  
  void *user_data; /* can be used by the GUI */
} PluginParam;

#define IVAL(p) ((p).param.ival.value)
#define SVAL(p) ((p).param.sval.value)
#define FVAL(p) ((p).param.fval.value)
#define BVAL(p) ((p).param.bval.value)
#define LVAL(p) ((p).param.slist.value)

#define FMIN(p) ((p).param.fval.min)
#define FMAX(p) ((p).param.fval.max)
#define FSTEP(p) ((p).param.fval.step)

#define IMIN(p) ((p).param.ival.min)
#define IMAX(p) ((p).param.ival.max)
#define ISTEP(p) ((p).param.ival.step)

void goom_secure_param(PluginParam *p);

void goom_secure_f_param(PluginParam *p, const char *name);
void goom_secure_i_param(PluginParam *p, const char *name);
void goom_secure_b_param(PluginParam *p, const char *name, int value);
void goom_secure_s_param(PluginParam *p, const char *name);

void goom_secure_f_feedback(PluginParam *p, const char *name);
void goom_secure_i_feedback(PluginParam *p, const char *name);

void goom_set_str_param_value(PluginParam *p, const char *str);
void goom_set_list_param_value(PluginParam *p, const char *str);
    
typedef struct _PARAMETERS {
  const char *name;
  const char *desc;
  int nbParams;
  PluginParam **params;
} PluginParameters;

void goom_plugin_parameters(PluginParameters *p, const char *name, int nb);
void goom_plugin_parameters_free(PluginParameters *p);

#define secure_param goom_secure_param
#define secure_f_param goom_secure_f_param
#define secure_i_param goom_secure_i_param
#define secure_b_param goom_secure_b_param
#define secure_s_param goom_secure_s_param
#define secure_f_feedback goom_secure_f_feedback
#define secure_i_feedback goom_secure_i_feedback
#define set_list_param_value goom_set_list_param_value
#define set_str_param_value goom_set_str_param_value
#define plugin_parameters goom_plugin_parameters

#endif

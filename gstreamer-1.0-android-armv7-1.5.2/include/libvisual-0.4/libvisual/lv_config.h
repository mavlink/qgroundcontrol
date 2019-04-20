/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_config.h,v 1.7 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_CONFIG_H
#define _LV_CONFIG_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_list.h>

VISUAL_BEGIN_DECLS

#define VISUAL_CONFIG_REGISTRY(obj)			(VISUAL_CHECK_CAST ((obj), VisConfigRegistry))
#define VISUAL_CONFIG_REGISTRYSECTION(obj)		(VISUAL_CHECK_CAST ((obj), VisConfigRegistrySection))

/**
 * Indicates at which version the config registry file format is.
 */
#define VISUAL_CONFIG_VERSION	"LV CONFIG FILE  3"

typedef struct _VisConfigRegistry VisConfigRegistry;
typedef struct _VisConfigRegistrySection VisConfigRegistrySection;

/**
 * The VisConfigRegistry structure is the container for a complete config registry.
 */
struct _VisConfigRegistry {
	VisObject	 object;	/**< The VisObject data. */

	char		*filename;	/**< Filename of the config file that is represented
					  *  by the VisConfigRegistry structure. */

	VisList		 sections;	/**< List containing all the sections within the registry. */
};

/**
 * The VisConfigRegistrySection structure is used as a section separator within VisConfigRegistry.
 */
struct _VisConfigRegistrySection {
	VisObject	 object;	/**< The VisObject data. */

	char		*name;		/**< The name of the section in the form of:
					  *  "domain:package:type:element:member".
					  *  For example "Libvisual:core:actor:oinksie:color mode" could
					  *  contain the "color mode" parameter off the oinksie actor plugin. */

	char		*data;		/**< The data that is associated with this registry element. */
	int		 datalength;	/**< Length of the data stored for this registry element. */
};

/* prototypes */
VisConfigRegistry *visual_config_registry_new (void);
VisConfigRegistry *visual_config_registry_open (const char *configfile);

VisConfigRegistrySection *visual_config_registry_find (VisConfigRegistry *registry, const char *name);
int visual_config_registry_remove (VisConfigRegistry *registry, const char *name);
int visual_config_registry_add (VisConfigRegistry *registry, VisConfigRegistrySection *rsection);

VisConfigRegistrySection *visual_config_registry_section_new (void);
VisConfigRegistrySection *visual_config_registry_section_open (const char *name, const char *configfile);

int visual_config_registry_write_by_data (VisConfigRegistry *registry, const char *name, const char *data, int datalength);
int visual_config_registry_write (VisConfigRegistry *registry, VisConfigRegistrySection *rsection);
int visual_config_registry_sync (VisConfigRegistry *registry);

VISUAL_END_DECLS

#endif /* _LV_CONFIG_H */

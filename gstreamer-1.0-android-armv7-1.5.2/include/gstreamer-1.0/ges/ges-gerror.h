/* GStreamer Editing Services
 * Copyright (C) 2013 Thibault Saunier <thibault.saunier@collabora.com>
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

#ifndef __GES_ERROR_H__
#define __GES_ERROR_H__

/**
 * SECTION: ges-gerror
 * @short_description: GError — Categorized error messages
 */

G_BEGIN_DECLS

/**
 * GES_ERROR:
 *
 * An error happened in GES
 */
#define GES_ERROR g_quark_from_static_string("GES_ERROR")

/**
 * GESError:
 * @GES_ERROR_ASSET_WRONG_ID: The ID passed is malformed
 * @GES_ERROR_ASSET_LOADING: An error happened while loading the asset
 * @GES_ERROR_FORMATTER_MALFORMED_INPUT_FILE: The formatted files was malformed
 */
typedef enum
{
  GES_ERROR_ASSET_WRONG_ID,
  GES_ERROR_ASSET_LOADING,
  GES_ERROR_FORMATTER_MALFORMED_INPUT_FILE,
} GESError;

G_END_DECLS
#endif /* __GES_ERROR_H__ */

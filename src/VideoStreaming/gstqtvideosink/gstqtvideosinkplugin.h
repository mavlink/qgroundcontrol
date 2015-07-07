/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011 Collabora Ltd. <info@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 2.1
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 *   @brief Extracted from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef GST_QT_VIDEO_SINK_PLUGIN_H
#define GST_QT_VIDEO_SINK_PLUGIN_H

#include <gst/gst.h>
#include <QtGlobal>

GST_DEBUG_CATEGORY_EXTERN(gst_qt5gstvideosink_debug);
#define GST_CAT_DEFAULT gst_qt5gstvideosink_debug


#define DEFINE_TYPE_FULL(cpp_type, type_name, parent_type, additional_initializations) \
    GType cpp_type::get_type() \
    { \
        static volatile gsize gonce_data = 0; \
        if (g_once_init_enter(&gonce_data)) { \
            GType type = 0; \
            GTypeInfo info; \
            info.class_size = sizeof(cpp_type##Class); \
            info.base_init = &cpp_type::base_init; \
            info.base_finalize = NULL; \
            info.class_init = &cpp_type::class_init; \
            info.class_finalize = NULL; \
            info.class_data = NULL; \
            info.instance_size = sizeof(cpp_type); \
            info.n_preallocs = 0; \
            info.instance_init = &cpp_type::init; \
            info.value_table = 0; \
            type = g_type_register_static(parent_type, g_intern_static_string(type_name), &info, (GTypeFlags)0); \
            additional_initializations(type); \
            g_once_init_leave(&gonce_data, (gsize) type); \
        } \
        return (GType) gonce_data; \
    }

// To allow qt4 and qt5 versions of the plugin to be installed at the same time,
// use a different name for their GType, so that the glib type system can handle
// both plugins being loaded by the gstreamer registry
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# define DEFINE_TYPE(cpp_type, parent_type) \
    DEFINE_TYPE_FULL(cpp_type, #cpp_type "_qt5", parent_type, Q_UNUSED)
# define DEFINE_TYPE_WITH_CODE(cpp_type, parent_type, additional_initializations) \
    DEFINE_TYPE_FULL(cpp_type, #cpp_type "_qt5", parent_type, additional_initializations)
#else
# define DEFINE_TYPE(cpp_type, parent_type) \
    DEFINE_TYPE_FULL(cpp_type, #cpp_type, parent_type, Q_UNUSED)
# define DEFINE_TYPE_WITH_CODE(cpp_type, parent_type, additional_initializations) \
    DEFINE_TYPE_FULL(cpp_type, #cpp_type, parent_type, additional_initializations)
#endif

inline bool qRealIsDouble() { return sizeof(qreal) == sizeof(double); }
#define G_TYPE_QREAL qRealIsDouble() ? G_TYPE_DOUBLE : G_TYPE_FLOAT


#endif

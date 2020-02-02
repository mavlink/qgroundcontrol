/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCSettings.h"

/// @file
///     @brief Core Plugin Interface for QGroundControl. Settings element.
///     @author Gus Grubba <gus@auterion.com>

QGCSettings::QGCSettings(QString title, QUrl url, QUrl icon)
    : _title(title)
    , _url(url)
    , _icon(icon)
{
}

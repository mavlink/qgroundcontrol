/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCOptions.h"

#include <QtQml>

/// @file
///     @brief Core Plugin Interface for QGroundControl - Application Options
///     @author Gus Grubba <gus@auterion.com>

QGCOptions::QGCOptions(QObject* parent)
    : QObject(parent)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

QColor QGCOptions::toolbarBackgroundLight() const
{
    return QColor(255,255,255);
}

QColor QGCOptions::toolbarBackgroundDark() const
{
    return QColor(0,0,0);
}

QGCFlyViewOptions* QGCOptions::flyViewOptions(void)
{
    if (!_defaultFlyViewOptions) {
        _defaultFlyViewOptions = new QGCFlyViewOptions(this);
    }
    return _defaultFlyViewOptions;
}

QGCFlyViewOptions::QGCFlyViewOptions(QGCOptions* options, QObject* parent)
    : QObject   (parent)
    , _options  (options)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

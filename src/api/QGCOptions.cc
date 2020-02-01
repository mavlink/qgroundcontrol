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
    , _defaultInstrumentWidget(nullptr)
{
    qmlRegisterUncreatableType<CustomInstrumentWidget>("QGroundControl", 1, 0, "CustomInstrumentWidget", "Reference only");
}

CustomInstrumentWidget*
QGCOptions::instrumentWidget()
{
    if(!_defaultInstrumentWidget) {
        _defaultInstrumentWidget = new CustomInstrumentWidget(this);
    }
    return _defaultInstrumentWidget;
}

QUrl
QGCOptions::mainToolbarUrl() const
{
    return QUrl(QStringLiteral("qrc:/toolbar/MainToolBar.qml"));
}

QUrl
QGCOptions::planToolbarUrl() const
{
    return QUrl(QStringLiteral("qrc:/qml/PlanToolBar.qml"));
}

QColor
QGCOptions::toolbarBackgroundLight() const
{
    return QColor(255,255,255,204);
}

QColor
QGCOptions::toolbarBackgroundDark() const
{
    return QColor(0,0,0,192);
}

QUrl
QGCOptions::planToolbarIndicatorsUrl() const
{
    return QUrl(QStringLiteral("PlanToolBar.qml"));
}


CustomInstrumentWidget::CustomInstrumentWidget(QObject* parent)
    : QObject(parent)
{
}


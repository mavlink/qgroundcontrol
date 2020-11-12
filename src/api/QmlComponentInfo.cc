/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QmlComponentInfo.h"

QmlComponentInfo::QmlComponentInfo(QString title, QUrl url, QUrl icon, QObject* parent)
    : QObject   (parent)
    , _title    (title)
    , _url      (url)
    , _icon     (icon)
{

}

/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QUrl>

/// Represents a Qml component which can be loaded from a resource.
class QmlComponentInfo : public QObject
{
    Q_OBJECT

public:
    QmlComponentInfo(QString title, QUrl url, QUrl icon = QUrl(), QObject* parent = nullptr);

    Q_PROPERTY(QString  title   READ title  CONSTANT)   ///< Title for page
    Q_PROPERTY(QUrl     url     READ url    CONSTANT)   ///< Qml source code
    Q_PROPERTY(QUrl     icon    READ icon   CONSTANT)   ///< Icon for page

    virtual QString title   () { return _title; }
    virtual QUrl    url     () { return _url;   }
    virtual QUrl    icon    () { return _icon;  }

protected:
    QString _title;
    QUrl    _url;
    QUrl    _icon;
};

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QUrl>

/// Represents a
class QmlPageInfo : public QObject
{
    Q_OBJECT

public:
    QmlPageInfo(QString title, QUrl url, QUrl icon = QUrl(), QObject* parent = NULL);

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

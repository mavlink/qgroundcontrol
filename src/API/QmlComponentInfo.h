/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QUrl>

Q_DECLARE_LOGGING_CATEGORY(QmlComponentInfoLog)

/// Represents a Qml component which can be loaded from a resource.
class QmlComponentInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  title   READ title  CONSTANT)
    Q_PROPERTY(QUrl     url     READ url    CONSTANT)
    Q_PROPERTY(QUrl     icon    READ icon   CONSTANT)

public:
    QmlComponentInfo(const QString &title, QUrl url, QUrl icon = QUrl(), QObject *parent = nullptr);
    ~QmlComponentInfo();

    const QString &title() const { return _title; }
    QUrl url() const { return _url; }
    QUrl icon() const { return _icon; }

protected:
    const QString _title; ///< Title for page
    const QUrl _url;      ///< Qml source code
    const QUrl _icon;     ///< Icon for page
};

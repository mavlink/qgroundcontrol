#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QUrl>

Q_DECLARE_LOGGING_CATEGORY(QmlComponentInfoLog)

/// Represents a Qml component which can be loaded from a resource.
class QmlComponentInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  title           READ title          CONSTANT)
    Q_PROPERTY(QUrl     url             READ url            CONSTANT)
    Q_PROPERTY(QUrl     icon            READ icon           CONSTANT)
    Q_PROPERTY(bool     requiresVehicle READ requiresVehicle CONSTANT)

public:
    QmlComponentInfo(const QString &title, QUrl url, QUrl icon = QUrl(), QObject *parent = nullptr, bool requiresVehicle = false);
    ~QmlComponentInfo();

    const QString &title() const { return _title; }
    QUrl url() const { return _url; }
    QUrl icon() const { return _icon; }
    bool requiresVehicle() const { return _requiresVehicle; }

protected:
    const QString _title;           ///< Title for page
    const QUrl _url;                ///< Qml source code
    const QUrl _icon;               ///< Icon for page
    const bool _requiresVehicle;    ///< Page requires connected vehicle
};

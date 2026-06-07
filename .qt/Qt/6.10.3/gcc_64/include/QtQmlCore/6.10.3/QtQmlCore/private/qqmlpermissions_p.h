// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLPERMISSIONS_P_H
#define QQMLPERMISSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmlglobal_p.h>

#if QT_CONFIG(permissions)

#include <QtQml/qqmlregistration.h>

#include <QtCore/qpermissions.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qproperty.h>
#include <QtCore/qglobal.h>

#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

#define QML_PERMISSION(Permission) \
    Q_OBJECT \
    QML_NAMED_ELEMENT(Permission) \
public: \
    Q_PROPERTY(Qt::PermissionStatus status READ status NOTIFY statusChanged) \
    Qt::PermissionStatus status() const { return qApp->checkPermission(m_permission); } \
    Q_SIGNAL void statusChanged(); \
    Q_INVOKABLE void request() { \
        const auto previousStatus = status(); \
        qApp->requestPermission(m_permission, this, \
            [this, previousStatus](const QPermission &permission) { \
                if (previousStatus != permission.status()) \
                    emit statusChanged(); \
        }); \
    } \
private: \
    Q##Permission m_permission; \
public:

#define QML_PERMISSION_PROPERTY(PropertyType, getterName, setterName) \
    Q_PROPERTY(PropertyType getterName READ getterName WRITE setterName NOTIFY getterName##Changed) \
    PropertyType getterName() const { return m_permission.getterName(); } \
    void setterName(const PropertyType &value) { \
        const auto previousValue = m_permission.getterName(); \
        const auto previousStatus = status(); \
        m_permission.setterName(value); \
        if (m_permission.getterName() != previousValue) { \
            emit getterName##Changed(); \
            if (status() != previousStatus) \
                emit statusChanged(); \
        } \
    } \
    Q_SIGNAL void getterName##Changed();


struct QQmlQLocationPermission : public QObject
{
    QML_PERMISSION(LocationPermission)
    QML_ADDED_IN_VERSION(6, 6)
    QML_EXTENDED_NAMESPACE(QLocationPermission)
    QML_PERMISSION_PROPERTY(QLocationPermission::Availability, availability, setAvailability)
    QML_PERMISSION_PROPERTY(QLocationPermission::Accuracy, accuracy, setAccuracy)
};

struct QQmlCalendarPermission : public QObject
{
    QML_PERMISSION(CalendarPermission)
    QML_ADDED_IN_VERSION(6, 6)
    QML_EXTENDED_NAMESPACE(QCalendarPermission)
    QML_PERMISSION_PROPERTY(QCalendarPermission::AccessMode, accessMode, setAccessMode)
};

struct QQmlContactsPermission : public QObject
{
    QML_PERMISSION(ContactsPermission)
    QML_ADDED_IN_VERSION(6, 6)
    QML_EXTENDED_NAMESPACE(QContactsPermission)
    QML_PERMISSION_PROPERTY(QContactsPermission::AccessMode, accessMode, setAccessMode)
};

struct QQmlBluetoothPermission : public QObject
{
    QML_PERMISSION(BluetoothPermission)
    QML_ADDED_IN_VERSION(6, 6)
    QML_EXTENDED_NAMESPACE(QBluetoothPermission)
    QML_PERMISSION_PROPERTY(QBluetoothPermission::CommunicationModes, communicationModes, setCommunicationModes)
};

struct QQmlCameraPermission : public QObject
{
    QML_PERMISSION(CameraPermission)
    QML_ADDED_IN_VERSION(6, 6)
};

struct QQmlMicrophonePermission : public QObject
{
    QML_PERMISSION(MicrophonePermission)
    QML_ADDED_IN_VERSION(6, 6)
};

QT_END_NAMESPACE

#endif // QT_CONFIG(permissions)

#endif // QQMLPERMISSIONS_P_H

// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLSYSTEMINFORMATION_P_H
#define QQMLSYSTEMINFORMATION_P_H

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

#include <QtCore/qobject.h>
#include <QtQmlCore/private/qqmlcoreglobal_p.h>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE
class Q_QMLCORE_EXPORT QQmlSystemInformation : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(SystemInformation)
    QML_ADDED_IN_VERSION(6, 4)

    Q_PROPERTY(int wordSize READ wordSize CONSTANT FINAL)
    Q_PROPERTY(QQmlSystemInformation::Endian byteOrder READ byteOrder CONSTANT FINAL)
    Q_PROPERTY(QString buildCpuArchitecture READ buildCpuArchitecture CONSTANT FINAL)
    Q_PROPERTY(QString currentCpuArchitecture READ currentCpuArchitecture CONSTANT FINAL)
    Q_PROPERTY(QString buildAbi READ buildAbi CONSTANT FINAL)
    Q_PROPERTY(QString kernelType READ kernelType CONSTANT FINAL)
    Q_PROPERTY(QString kernelVersion READ kernelVersion CONSTANT FINAL)
    Q_PROPERTY(QString productType READ productType CONSTANT FINAL)
    Q_PROPERTY(QString productVersion READ productVersion CONSTANT FINAL)
    Q_PROPERTY(QString prettyProductName READ prettyProductName CONSTANT FINAL)
    Q_PROPERTY(QString machineHostName READ machineHostName CONSTANT FINAL)
    Q_PROPERTY(QByteArray machineUniqueId READ machineUniqueId CONSTANT FINAL)
    Q_PROPERTY(QByteArray bootUniqueId READ bootUniqueId CONSTANT FINAL)

public:
    enum class Endian { Big, Little };
    Q_ENUM(Endian)

    explicit QQmlSystemInformation(QObject *parent = nullptr);

    int wordSize() const;
    Endian byteOrder() const;
    QString buildCpuArchitecture() const;
    QString currentCpuArchitecture() const;
    QString buildAbi() const;
    QString kernelType() const;
    QString kernelVersion() const;
    QString productType() const;
    QString productVersion() const;
    QString prettyProductName() const;
    QString machineHostName() const;
    QByteArray machineUniqueId() const;
    QByteArray bootUniqueId() const;
};
QT_END_NAMESPACE

#endif // QQMLSYSTEMINFORMATION_P_H

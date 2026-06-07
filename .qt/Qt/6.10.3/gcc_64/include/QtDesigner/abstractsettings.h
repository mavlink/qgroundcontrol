// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTSETTINGS_P_H
#define ABSTRACTSETTINGS_P_H

#include <QtDesigner/sdk_global.h>

#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QString;

class QDESIGNER_SDK_EXPORT QDesignerSettingsInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerSettingsInterface)

    QDesignerSettingsInterface() = default;
    virtual ~QDesignerSettingsInterface() = default;

    virtual void beginGroup(const QString &prefix) = 0;
    virtual void endGroup() = 0;

    virtual bool contains(const QString &key) const = 0;
    virtual void setValue(const QString &key, const QVariant &value) = 0;
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const = 0;
    virtual void remove(const QString &key) = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTSETTINGS_P_H

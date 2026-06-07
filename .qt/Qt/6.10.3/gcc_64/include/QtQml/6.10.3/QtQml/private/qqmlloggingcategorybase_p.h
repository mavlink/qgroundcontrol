// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLOGGINGCATEGORYBASE_P_H
#define QQMLLOGGINGCATEGORYBASE_P_H

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

#include <QtQml/qqml.h>

#include <QtCore/qobject.h>
#include <QtCore/qloggingcategory.h>

#include <memory>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlLoggingCategoryBase : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    QQmlLoggingCategoryBase(QObject *parent = nullptr) : QObject(parent) {}

    const QLoggingCategory *category() {
        forceCompletion();
        return m_category.get();
    }
    void setCategory(const char *name, QtMsgType type)
    {
        m_category = std::make_unique<QLoggingCategory>(name, type);
    }

    virtual void forceCompletion() = 0;

private:
    std::unique_ptr<QLoggingCategory> m_category;
};

QT_END_NAMESPACE

#endif // QQMLLOGGINGCATEGORYBASE_P_H

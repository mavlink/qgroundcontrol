// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTREEROW_P_H
#define QQMLTREEROW_P_H

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

#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

#include <QtCore/qmap.h>
#include <QtCore/qvariant.h>

#include <memory>
#include <vector>

QT_REQUIRE_CONFIG(qml_tree_model);

QT_BEGIN_NAMESPACE

class QQmlTreeRow
{
public:
    explicit QQmlTreeRow(QQmlTreeRow *parentItem = nullptr);
    explicit QQmlTreeRow(const QVariant &data, QQmlTreeRow *parentItem = nullptr);
    explicit QQmlTreeRow(const QVariantMap &data, QQmlTreeRow *parentItem = nullptr);

    QQmlTreeRow *parent() const { return m_parent; }
    void setParent(QQmlTreeRow *parent) { m_parent = parent; }

    const QQmlTreeRow *getRow(int i) const { return m_children[i].get(); }
    void addChild(QQmlTreeRow *child);
    size_t rowCount() const { return m_children.size(); }
    int subTreeSize() const;

    QVariantMap data() const { return dataMap; }
    QVariant data(const QString &key) const { return dataMap[key]; }
    const std::vector<std::unique_ptr<QQmlTreeRow>>& children() const { return m_children; }
    void removeChild(std::vector<std::unique_ptr<QQmlTreeRow>>::const_iterator &child);
    void removeChildAt(int i);
    void setData(const QVariant &data);
    void setData(const QVariantMap &data);
    void setField(const QString &key, const QVariant &value);
    QVariant toVariant() const;
private:
    void unpackVariantMap(const QVariantMap &dataMap);

    QQmlTreeRow *m_parent;
    std::vector<std::unique_ptr<QQmlTreeRow>> m_children;
    QVariantMap dataMap;
};

QT_END_NAMESPACE

#endif // QQMLTREEROW_P_H

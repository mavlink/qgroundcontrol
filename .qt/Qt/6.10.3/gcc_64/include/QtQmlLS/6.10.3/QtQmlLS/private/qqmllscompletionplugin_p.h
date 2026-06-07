// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLSCOMPLETIONPLUGIN_H
#define QQMLLSCOMPLETIONPLUGIN_H

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

#include <iterator>

#include <QtQmlDom/private/qqmldomelements_p.h>
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>

QT_BEGIN_NAMESPACE

class QQmlLSCompletionPlugin
{
public:
    QQmlLSCompletionPlugin() = default;
    virtual ~QQmlLSCompletionPlugin() = default;

    using BackInsertIterator = std::back_insert_iterator<QList<QLspSpecification::CompletionItem>>;

    virtual void suggestSnippetsForLeftHandSideOfBinding(const QQmlJS::Dom::DomItem &items,
                                                         BackInsertIterator result) const = 0;

    virtual void suggestSnippetsForRightHandSideOfBinding(const QQmlJS::Dom::DomItem &items,
                                                          BackInsertIterator result) const = 0;
};

QT_END_NAMESPACE

#endif // QQMLLSCOMPLETIONPLUGIN_H

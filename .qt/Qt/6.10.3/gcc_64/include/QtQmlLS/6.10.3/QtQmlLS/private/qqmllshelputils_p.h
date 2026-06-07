// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLSHELPUTILS_P_H
#define QQMLLSHELPUTILS_P_H

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

#include <QtQmlLS/private/qqmllshelpplugininterface_p.h>
#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlLS/private/qdochtmlparser_p.h>
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>

#include <vector>

QT_BEGIN_NAMESPACE

class HelpManager final
{
public:
    HelpManager();
    void setDocumentationRootPath(const QString &path);
    [[nodiscard]] QString documentationRootPath() const;
    [[nodiscard]] std::optional<QByteArray> documentationForItem(
            const QQmlJS::Dom::DomItem &file, QLspSpecification::Position position);

private:
    [[nodiscard]] std::optional<QByteArray> extractDocumentationForIdentifiers(const QQmlJS::Dom::DomItem &item,
                                                QQmlLSUtils::ExpressionType expr) const;
    [[nodiscard]] std::optional<QByteArray> extractDocumentationForDomElements(
            const QQmlJS::Dom::DomItem &item) const;
    [[nodiscard]] std::optional<QByteArray> extractDocumentation(
            const QQmlJS::Dom::DomItem &item) const;
    [[nodiscard]] std::optional<QByteArray> tryExtract(ExtractDocumentation &extractor,
                                                       const std::vector<QQmlLSHelpProviderBase::DocumentLink> &links,
                                                       const QString &name) const;
    [[nodiscard]] std::vector<QQmlLSHelpProviderBase::DocumentLink>
    collectDocumentationLinks(const QQmlJS::Dom::DomItem &item, const QQmlJSScope::ConstPtr &scope,
                              const QString &name) const;
    void registerDocumentations(const QStringList &docs) const;
    std::unique_ptr<QQmlLSHelpProviderBase> m_helpPlugin;
    QString m_docRootPath;
    QHash<QString, QString> m_cppTypesToQmlTypes;
};

QT_END_NAMESPACE

#endif // QQMLLSHELPUTILS_P_H

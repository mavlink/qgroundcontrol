// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPELOADERQMLDIRCONTENT_P_H
#define QQMLTYPELOADERQMLDIRCONTENT_P_H

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

#include <private/qqmldirparser_p.h>

QT_BEGIN_NAMESPACE

class QQmlError;
class QQmlTypeLoaderQmldirContent
{
private:
    friend class QQmlTypeLoader;

    void setContent(const QString &location, const QString &content);
    void setError(const QQmlError &);

public:
    QQmlTypeLoaderQmldirContent() = default;
    QQmlTypeLoaderQmldirContent(const QQmlTypeLoaderQmldirContent &) = default;
    QQmlTypeLoaderQmldirContent &operator=(const QQmlTypeLoaderQmldirContent &) = default;

    bool hasContent() const { return m_hasContent; }
    bool hasError() const { return m_parser.hasError(); }
    QList<QQmlError> errors(const QString &uri, const QUrl &url) const;

    QString typeNamespace() const { return m_parser.typeNamespace(); }

    QQmlDirComponents components() const { return m_parser.components(); }
    QQmlDirScripts scripts() const { return m_parser.scripts(); }
    QQmlDirPlugins plugins() const { return m_parser.plugins(); }
    QQmlDirImports imports() const { return m_parser.imports(); }

    QString qmldirLocation() const { return m_location; }
    QString preferredPath() const { return m_parser.preferredPath(); }

    bool hasRedirection() const
    {
        const QString preferred = preferredPath();
        return !preferred.isEmpty()
                && preferred != QStringView(m_location).chopped(strlen("qmldir"));
    }

    bool designerSupported() const { return m_parser.designerSupported(); }
    bool hasTypeInfo() const { return !m_parser.typeInfos().isEmpty(); }

private:
    QQmlDirParser m_parser;
    QString m_location;
    bool m_hasContent = false;
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADERQMLDIRCONTENT_P_H

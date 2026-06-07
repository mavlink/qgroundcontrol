// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLLSHELPPLUGININTERFACE_H
#define QQMLLSHELPPLUGININTERFACE_H

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

#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <vector>

QT_BEGIN_NAMESPACE

class QQmlLSHelpProviderBase
{
public:
    struct DocumentLink
    {
        QUrl url;
        QString title;
    };

public:
    virtual ~QQmlLSHelpProviderBase() = default;
    virtual bool registerDocumentation(const QString &documentationFileName) = 0;
    [[nodiscard]] virtual QByteArray fileData(const QUrl &url) const = 0;
    [[nodiscard]] virtual std::vector<DocumentLink> documentsForIdentifier(const QString &id) const = 0;
    [[nodiscard]] virtual std::vector<DocumentLink>
    documentsForIdentifier(const QString &id, const QString &filterName) const = 0;
    [[nodiscard]] virtual std::vector<DocumentLink> documentsForKeyword(const QString &keyword) const = 0;
    [[nodiscard]] virtual std::vector<DocumentLink>
    documentsForKeyword(const QString &keyword, const QString &filterName) const = 0;
    [[nodiscard]] virtual QStringList registeredNamespaces() const = 0;
    [[nodiscard]] virtual QString error() const = 0;
};

class QQmlLSHelpPluginInterface
{
public:
    QQmlLSHelpPluginInterface() = default;
    virtual ~QQmlLSHelpPluginInterface() = default;
    Q_DISABLE_COPY_MOVE(QQmlLSHelpPluginInterface)

    virtual std::unique_ptr<QQmlLSHelpProviderBase> initialize(const QString &collectionFile,
                                                               QObject *parent) = 0;
};

#define QQmlLSHelpPluginInterface_iid "org.qt-project.Qt.QmlLS.HelpPlugin/1.0"
Q_DECLARE_INTERFACE(QQmlLSHelpPluginInterface, QQmlLSHelpPluginInterface_iid)

QT_END_NAMESPACE

#endif // QQMLLSHELPPLUGININTERFACE_H

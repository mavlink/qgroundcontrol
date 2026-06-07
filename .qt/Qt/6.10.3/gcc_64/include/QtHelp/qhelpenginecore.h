// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPENGINECORE_H
#define QHELPENGINECORE_H

#include <QtHelp/qhelp_global.h>
#include <QtHelp/qhelpcontentitem.h>

#if QT_CONFIG(future)
#include <QtCore/qfuture.h>
#endif

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QHelpEngineCorePrivate;
class QHelpFilterEngine;
struct QHelpLink;

class QHELP_EXPORT QHelpEngineCore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoSaveFilter READ autoSaveFilter WRITE setAutoSaveFilter)
    Q_PROPERTY(QString collectionFile READ collectionFile WRITE setCollectionFile)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
#if QT_DEPRECATED_SINCE(5, 15)
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter)
#endif

public:
    explicit QHelpEngineCore(const QString &collectionFile, QObject *parent = nullptr);
    virtual ~QHelpEngineCore();

    bool isReadOnly() const;
    void setReadOnly(bool enable);

    QHelpFilterEngine *filterEngine() const;

    bool setupData();

    QString collectionFile() const;
    void setCollectionFile(const QString &fileName);

    bool copyCollectionFile(const QString &fileName);

    static QString namespaceName(const QString &documentationFileName);
    bool registerDocumentation(const QString &documentationFileName);
    bool unregisterDocumentation(const QString &namespaceName);
    QString documentationFileName(const QString &namespaceName);
    QStringList registeredDocumentations() const;
    QByteArray fileData(const QUrl &url) const;

// #if QT_DEPRECATED_SINCE(5,13)
    QStringList customFilters() const;
    bool removeCustomFilter(const QString &filterName);
    bool addCustomFilter(const QString &filterName,
        const QStringList &attributes);

    QStringList filterAttributes() const;
    QStringList filterAttributes(const QString &filterName) const;

    QString currentFilter() const;
    void setCurrentFilter(const QString &filterName);

    QList<QStringList> filterAttributeSets(const QString &namespaceName) const;
    QList<QUrl> files(const QString namespaceName, const QStringList &filterAttributes,
                      const QString &extensionFilter = {});
// #endif

    QList<QUrl> files(const QString namespaceName, const QString &filterName,
                      const QString &extensionFilter = {});
    QUrl findFile(const QUrl &url) const;

    QList<QHelpLink> documentsForIdentifier(const QString &id) const;
    QList<QHelpLink> documentsForIdentifier(const QString &id, const QString &filterName) const;
    QList<QHelpLink> documentsForKeyword(const QString &keyword) const;
    QList<QHelpLink> documentsForKeyword(const QString &keyword, const QString &filterName) const;

    bool removeCustomValue(const QString &key);
    QVariant customValue(const QString &key, const QVariant &defaultValue = {}) const;
    bool setCustomValue(const QString &key, const QVariant &value);

    static QVariant metaData(const QString &documentationFileName, const QString &name);

    QString error() const;

    void setAutoSaveFilter(bool save);
    bool autoSaveFilter() const;

    void setUsesFilterEngine(bool uses);
    bool usesFilterEngine() const;

#if QT_CONFIG(future)
    QFuture<std::shared_ptr<QHelpContentItem>> requestContentForCurrentFilter() const;
    QFuture<std::shared_ptr<QHelpContentItem>> requestContent(const QString &filter) const;

    QFuture<QStringList> requestIndexForCurrentFilter() const;
    QFuture<QStringList> requestIndex(const QString &filter) const;
#endif

Q_SIGNALS:
    void setupStarted();
    void setupFinished();
    void warning(const QString &msg);

// #if QT_DEPRECATED_SINCE(5,13)
    void currentFilterChanged(const QString &newFilter);
    void readersAboutToBeInvalidated();
// #endif

protected:
#if QT_DEPRECATED_SINCE(6, 8)
    QHelpEngineCore(QHelpEngineCorePrivate *helpEngineCorePrivate, QObject *parent);
#endif

private:
    QHelpEngineCorePrivate *d;
};

QT_END_NAMESPACE

#endif // QHELPENGINECORE_H

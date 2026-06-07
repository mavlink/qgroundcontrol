// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHELPFILTERENGINE_H
#define QHELPFILTERENGINE_H

#include <QtHelp/qhelp_global.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QHelpCollectionHandler;
class QHelpEngineCore;
class QHelpFilterData;
class QHelpFilterEnginePrivate;
template <class K, class T>
class QMap;
class QVersionNumber;

class QHELP_EXPORT QHelpFilterEngine : public QObject
{
    Q_OBJECT
public:
    QMap<QString, QString> namespaceToComponent() const;
    QMap<QString, QVersionNumber> namespaceToVersion() const;

    QStringList filters() const;

    QString activeFilter() const;
    bool setActiveFilter(const QString &filterName);

    QStringList availableComponents() const;
    QList<QVersionNumber> availableVersions() const;

    QHelpFilterData filterData(const QString &filterName) const;
    bool setFilterData(const QString &filterName, const QHelpFilterData &filterData);

    bool removeFilter(const QString &filterName);

    QStringList namespacesForFilter(const QString &filterName) const;

    QStringList indices() const;
    QStringList indices(const QString &filterName) const;

Q_SIGNALS:
    void filterActivated(const QString &newFilter);

protected:
    explicit QHelpFilterEngine(QHelpEngineCore *helpEngine);
    virtual ~QHelpFilterEngine();

private:
    void setCollectionHandler(QHelpCollectionHandler *collectionHandler);

    QHelpFilterEnginePrivate *d;
    friend class QHelpEngineCorePrivate;
};

QT_END_NAMESPACE

#endif // QHELPFILTERENGINE_H

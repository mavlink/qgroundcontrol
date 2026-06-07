// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLXMLLISTMODEL_H
#define QQMLXMLLISTMODEL_H

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

#include <private/qflatmap_p.h>
#include <private/qtqmlxmllistmodelglobal_p.h>

#include <QtQml/qqmllist.h>
#include <QtQml/qqmlparserstatus.h>

#include <QtQmlIntegration/qqmlintegration.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qfuture.h>
#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_network)
class QNetworkReply;
#endif

class QXmlStreamReader;
class QQmlContext;
struct QQmlXmlListModelQueryJob
{
    int queryId;
    QByteArray data;
    QString query;
    QStringList roleNames;
    QStringList elementNames;
    QStringList elementAttributes;
    QList<void *> roleQueryErrorId;
};
struct QQmlXmlListModelQueryResult
{
    Q_GADGET
    QML_ANONYMOUS
public:
    int queryId;
    QList<QFlatMap<int, QString>> data;
    QList<std::pair<void *, QString>> errors;
};

class Q_QMLXMLLISTMODEL_EXPORT QQmlXmlListModelRole : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString elementName READ elementName WRITE setElementName NOTIFY elementNameChanged)
    Q_PROPERTY(QString attributeName READ attributeName WRITE setAttributeName NOTIFY
                       attributeNameChanged)
    QML_NAMED_ELEMENT(XmlListModelRole)

public:
    QQmlXmlListModelRole() = default;
    ~QQmlXmlListModelRole() = default;

    QString name() const;
    void setName(const QString &name);
    QString elementName() const;
    void setElementName(const QString &name);
    QString attributeName() const;
    void setAttributeName(const QString &attributeName);
    bool isValid() const;

Q_SIGNALS:
    void nameChanged();
    void elementNameChanged();
    void attributeNameChanged();

private:
    QString m_name;
    QString m_elementName;
    QString m_attributeName;
};

class QQmlXmlListModelQueryExecutor;

class Q_QMLXMLLISTMODEL_EXPORT QQmlXmlListModel : public QAbstractListModel,
                                                          public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QQmlListProperty<QQmlXmlListModelRole> roles READ roleObjects)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    QML_NAMED_ELEMENT(XmlListModel)
    Q_CLASSINFO("DefaultProperty", "roles")

public:
    QQmlXmlListModel(QObject *parent = nullptr);
    ~QQmlXmlListModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    QUrl source() const;
    void setSource(const QUrl &);

    QString query() const;
    void setQuery(const QString &);

    QQmlListProperty<QQmlXmlListModelRole> roleObjects();

    void appendRole(QQmlXmlListModelRole *);
    void clearRole();

    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    Status status() const;
    qreal progress() const;

    Q_INVOKABLE QString errorString() const;

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void statusChanged(QQmlXmlListModel::Status);
    void progressChanged(qreal progress);
    void countChanged();
    void sourceChanged();
    void queryChanged();

public Q_SLOTS:
    void reload();

private Q_SLOTS:
#if QT_CONFIG(qml_network)
    void requestFinished();
#endif
    void requestProgress(qint64, qint64);
    void dataCleared();
    void queryCompleted(const QQmlXmlListModelQueryResult &);
    void queryError(void *object, const QString &error);

private:
    Q_DISABLE_COPY(QQmlXmlListModel)

    void notifyQueryStarted(bool remoteSource);

    static void appendRole(QQmlListProperty<QQmlXmlListModelRole> *, QQmlXmlListModelRole *);
    static void clearRole(QQmlListProperty<QQmlXmlListModelRole> *);

    void tryExecuteQuery(const QByteArray &data);

    QQmlXmlListModelQueryJob createJob(const QByteArray &data);
    int nextQueryId();

#if QT_CONFIG(qml_network)
    void deleteReply();

    QNetworkReply *m_reply = nullptr;
#endif

    int m_size = 0;
    QUrl m_source;
    QString m_query;
    QStringList m_roleNames;
    QList<int> m_roles;
    QList<QQmlXmlListModelRole *> m_roleObjects;
    QList<QFlatMap<int, QString>> m_data;
    bool m_isComponentComplete = true;
    Status m_status = QQmlXmlListModel::Null;
    QString m_errorString;
    qreal m_progress = 0;
    int m_queryId = -1;
    int m_nextQueryIdGenerator = -1;
    int m_highestRole = Qt::UserRole;
    using ResultFutureWatcher = QFutureWatcher<QQmlXmlListModelQueryResult>;
    QFlatMap<int, ResultFutureWatcher *> m_watchers;
};

class QQmlXmlListModelQueryRunnable : public QRunnable
{
public:
    explicit QQmlXmlListModelQueryRunnable(QQmlXmlListModelQueryJob &&job);
    void run() override;

    QFuture<QQmlXmlListModelQueryResult> future() const;

private:
    void doQueryJob(QQmlXmlListModelQueryResult *currentResult);
    void processElement(QQmlXmlListModelQueryResult *currentResult, const QString &element,
                        QXmlStreamReader &reader);
    void readSubTree(const QString &prefix, QXmlStreamReader &reader,
                     QFlatMap<int, QString> &results, QList<std::pair<void *, QString>> *errors);

    QQmlXmlListModelQueryJob m_job;
    QPromise<QQmlXmlListModelQueryResult> m_promise;
};

QT_END_NAMESPACE

#endif // QQMLXMLLISTMODEL_H

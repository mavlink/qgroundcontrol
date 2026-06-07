// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLAPPLICATIONENGINE_H
#define QQMLAPPLICATIONENGINE_H

#include <QtQml/qqmlengine.h>

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QQmlApplicationEnginePrivate;
class Q_QML_EXPORT QQmlApplicationEngine : public QQmlEngine
{
    Q_OBJECT
public:
    QQmlApplicationEngine(QObject *parent = nullptr);
    QQmlApplicationEngine(const QUrl &url, QObject *parent = nullptr);
    explicit QQmlApplicationEngine(QAnyStringView uri, QAnyStringView typeName,
                                   QObject *parent = nullptr);
    QQmlApplicationEngine(const QString &filePath, QObject *parent = nullptr);
    ~QQmlApplicationEngine() override;

    QList<QObject*> rootObjects() const;

public Q_SLOTS:
    void load(const QUrl &url);
    void load(const QString &filePath);
    void loadFromModule(QAnyStringView uri, QAnyStringView typeName);
    void setInitialProperties(const QVariantMap &initialProperties);
    void setExtraFileSelectors(const QStringList &extraFileSelectors);
    void loadData(const QByteArray &data, const QUrl &url = QUrl());

Q_SIGNALS:
    void objectCreated(QObject *object, const QUrl &url);
    void objectCreationFailed(const QUrl &url);

private:
    Q_DISABLE_COPY(QQmlApplicationEngine)
    Q_PRIVATE_SLOT(d_func(), void _q_loadTranslations())
    Q_DECLARE_PRIVATE(QQmlApplicationEngine)
};

QT_END_NAMESPACE

#endif

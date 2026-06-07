// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLAPPLICATIONENGINE_P_H
#define QQMLAPPLICATIONENGINE_P_H

#include "qqmlapplicationengine.h"
#include "qqmlengine_p.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QLibraryInfo>

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

QT_BEGIN_NAMESPACE

class QFileSelector;
class Q_QML_EXPORT QQmlApplicationEnginePrivate : public QQmlEnginePrivate
{
    Q_DECLARE_PUBLIC(QQmlApplicationEngine)
public:
    QQmlApplicationEnginePrivate(QQmlEngine *e);
    ~QQmlApplicationEnginePrivate();
    void ensureInitialized();
    void init();
    void cleanUp();

    void startLoad(const QUrl &url, const QByteArray &data = QByteArray(), bool dataFlag = false);
    void startLoad(QAnyStringView uri, QAnyStringView type);
    void _q_loadTranslations();
    void finishLoad(QQmlComponent *component);
    void ensureLoadingFinishes(QQmlComponent *component);
    void updateTranslationDirectory(const QUrl &url);

    QList<QObject *> objects;
    QVariantMap initialProperties;
    QStringList extraFileSelectors;
    QString translationsDirectory;
#if QT_CONFIG(translation)
    std::unique_ptr<QTranslator> activeTranslator;
#endif
    bool isInitialized = false;
};

QT_END_NAMESPACE

#endif

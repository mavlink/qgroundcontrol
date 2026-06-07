// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QQMLDEBUGTRANSLATIONCLIENT_P_H
#define QQMLDEBUGTRANSLATIONCLIENT_P_H

#include "qqmldebugclient_p.h"

#include <QtCore/qvector.h>
#include <private/qqmldebugtranslationprotocol_p.h>

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

class QQmlDebugTranslationClient : public QQmlDebugClient
{
    Q_OBJECT

public:
    explicit QQmlDebugTranslationClient(QQmlDebugConnection *client);
    ~QQmlDebugTranslationClient() = default;

    virtual void messageReceived(const QByteArray &message) override;
    bool languageChanged = false;
    QVector<QQmlDebugTranslation::TranslationIssue> translationIssues;
    QVector<QQmlDebugTranslation::QmlElement> qmlElements;
    QVector<QQmlDebugTranslation::QmlState> qmlStates;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGTRANSLATIONCLIENT_P_H

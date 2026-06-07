// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2013 Teo Mrnjavac <teo@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMSESSIONMANAGER_H
#define QPLATFORMSESSIONMANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qnamespace.h>

#include <QtGui/qsessionmanager.h>

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPlatformSessionManager
{
public:
    Q_DISABLE_COPY_MOVE(QPlatformSessionManager)

    explicit QPlatformSessionManager(const QString &id, const QString &key);
    virtual ~QPlatformSessionManager();

    virtual QString sessionId() const;
    virtual QString sessionKey() const;

    virtual bool allowsInteraction();
    virtual bool allowsErrorInteraction();
    virtual void release();

    virtual void cancel();

    virtual void setRestartHint(QSessionManager::RestartHint restartHint);
    virtual QSessionManager::RestartHint restartHint() const;

    virtual void setRestartCommand(const QStringList &command);
    virtual QStringList restartCommand() const;
    virtual void setDiscardCommand(const QStringList &command);
    virtual QStringList discardCommand() const;

    virtual void setManagerProperty(const QString &name, const QString &value);
    virtual void setManagerProperty(const QString &name, const QStringList &value);

    virtual bool isPhase2() const;
    virtual void requestPhase2();

    void appCommitData();
    void appSaveState();

protected:
    QString m_sessionId;
    QString m_sessionKey;

private:
    QStringList m_restartCommand;
    QStringList m_discardCommand;
    QSessionManager::RestartHint m_restartHint;
};

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER

#endif // QPLATFORMSESSIONMANAGER_H

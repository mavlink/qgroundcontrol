// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLANGUAGESERVER_P_H
#define QLANGUAGESERVER_P_H

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

#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtLanguageServer/private/qlspnotifysignals_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

class QLanguageServer;
class QLanguageServerPrivate;
Q_DECLARE_LOGGING_CATEGORY(lspServerLog)

class QLanguageServerModule : public QObject
{
    Q_OBJECT
public:
    QLanguageServerModule(QObject *parent = nullptr) : QObject(parent) { }
    virtual QString name() const = 0;
    virtual void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) = 0;
    virtual void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                                   QLspSpecification::InitializeResult &) = 0;
};

class QLanguageServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RunStatus runStatus READ runStatus NOTIFY runStatusChanged)
    Q_PROPERTY(bool isInitialized READ isInitialized)
public:
    QLanguageServer(const QJsonRpcTransport::DataHandler &h, QObject *parent = nullptr);
    enum class RunStatus {
        NotSetup,
        SettingUp,
        DidSetup,
        Initializing,
        DidInitialize, // normal state of execution
        WaitPending,
        Stopping,
        WaitingForExit,
        Stopped
    };
    Q_ENUM(RunStatus)

    QLanguageServerProtocol *protocol();
    void finishSetup();
    void registerHandlers(QLanguageServerProtocol *protocol);
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &serverInfo);
    void addServerModule(QLanguageServerModule *serverModule);
    QLanguageServerModule *moduleByName(const QString &n) const;
    QLspNotifySignals *notifySignals();

    // API
    RunStatus runStatus() const;
    bool isInitialized() const;
    bool isRequestCanceled(const QJsonRpc::IdType &id) const;
    const QLspSpecification::InitializeParams &clientInfo() const;
    const QLspSpecification::InitializeResult &serverInfo() const;

public Q_SLOTS:
    void receiveData(const QByteArray &d, bool isEndOfMessage);
Q_SIGNALS:
    void runStatusChanged(RunStatus);
    void clientInitialized(QLanguageServer *server);
    void shutdown();
    void exit();
    void lifecycleError();
    void readNextMessage();

private:
    void registerMethods(QJsonRpc::TypedRpc &typedRpc);
    void executeShutdown();
    Q_DECLARE_PRIVATE(QLanguageServer)
};

QT_END_NAMESPACE

#endif // QLANGUAGESERVER_P_H

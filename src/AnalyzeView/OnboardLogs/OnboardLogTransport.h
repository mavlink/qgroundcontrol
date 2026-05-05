#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QmlObjectListModel;

/// Common contract for an onboard-log transport (LOG protocol or MAVLink FTP).
/// Concrete subclasses (LogProtocolTransport, FtpTransport) are owned by
/// OnboardLogController, which selects one per active vehicle and forwards the
/// QML-facing surface. The base is also the type QML binds against.
class OnboardLogTransport : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel* model READ model NOTIFY modelChanged)
    Q_PROPERTY(bool requestingList READ requestingList NOTIFY requestingListChanged)
    Q_PROPERTY(bool downloadingLogs READ downloadingLogs NOTIFY downloadingLogsChanged)
    Q_PROPERTY(bool canErase READ canErase NOTIFY canEraseChanged)
    Q_PROPERTY(QString transportName READ transportName NOTIFY transportNameChanged)

public:
    ~OnboardLogTransport() override;

    virtual QmlObjectListModel* model() const { return _logEntriesModel; }

    virtual bool requestingList() const { return _requestingLogEntries; }

    virtual bool downloadingLogs() const { return _downloadingLogs; }

    virtual bool canErase() const { return false; }

    virtual QString transportName() const = 0;

    Q_INVOKABLE virtual void refresh() = 0;
    Q_INVOKABLE virtual void download(const QString& path = QString()) = 0;
    Q_INVOKABLE virtual void cancel() = 0;

    Q_INVOKABLE virtual void eraseAll() {}

signals:
    void modelChanged();
    void requestingListChanged();
    void downloadingLogsChanged();
    void canEraseChanged();
    void transportNameChanged();
    void selectionChanged();

protected:
    explicit OnboardLogTransport(QObject* parent = nullptr);

    QmlObjectListModel* _logEntriesModel = nullptr;
    bool _requestingLogEntries = false;
    bool _downloadingLogs = false;
};

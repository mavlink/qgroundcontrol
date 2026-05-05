#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <cstdint>

class FTPManager;

/// Scoped handle for one FTPManager operation.
///
/// Jobs bind progress, completion, and cancellation to the request which
/// created them. FTPManager still exposes its legacy manager-wide signals for
/// compatibility, but new consumers should connect to the returned job.
class FTPJob : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FTPJob)

public:
    enum class Type : uint8_t
    {
        Download,
        Upload,
        ListDirectory,
        Delete,
    };
    Q_ENUM(Type)

    ~FTPJob() override;

    Type type() const { return _type; }

    bool active() const { return !_finished && !_manager.isNull(); }

    Q_INVOKABLE void cancel();

signals:
    void progress(float value);

protected:
    FTPJob(FTPManager* manager, Type type);

private:
    friend class FTPManager;

    void _finish();

    void _emitProgress(float value) { emit progress(value); }

    QPointer<FTPManager> _manager;
    const Type _type;
    bool _finished = false;
};

class FTPDownloadJob final : public FTPJob
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FTPDownloadJob)

signals:
    void finished(const QString& file, const QString& error, const QString& warning);

private:
    friend class FTPManager;
    explicit FTPDownloadJob(FTPManager* manager);

    void _emitFinished(const QString& file, const QString& error, const QString& warning)
    {
        emit finished(file, error, warning);
    }
};

class FTPUploadJob final : public FTPJob
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FTPUploadJob)

signals:
    void finished(const QString& file, const QString& error);

private:
    friend class FTPManager;
    explicit FTPUploadJob(FTPManager* manager);

    void _emitFinished(const QString& file, const QString& error) { emit finished(file, error); }
};

class FTPListDirectoryJob final : public FTPJob
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FTPListDirectoryJob)

signals:
    void finished(const QStringList& entries, const QString& error, bool truncated);

private:
    friend class FTPManager;
    explicit FTPListDirectoryJob(FTPManager* manager);

    void _emitFinished(const QStringList& entries, const QString& error, bool truncated)
    {
        emit finished(entries, error, truncated);
    }
};

class FTPDeleteJob final : public FTPJob
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FTPDeleteJob)

signals:
    void finished(const QString& file, const QString& error);

private:
    friend class FTPManager;
    explicit FTPDeleteJob(FTPManager* manager);

    void _emitFinished(const QString& file, const QString& error) { emit finished(file, error); }
};

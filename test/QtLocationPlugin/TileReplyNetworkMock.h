#pragma once

#include <cstring>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

// Network stand-ins for tests of tile fetching: no request leaves the process.

/// Finishes with a canned body, error and HTTP status; a held reply stays open until finishNow()
class TileReplyCannedReply : public QNetworkReply
{
public:
    TileReplyCannedReply(const QNetworkRequest& request, const QByteArray& body, NetworkError cannedError,
                         int httpStatus, bool hold, QObject* parent)
        : QNetworkReply(parent)
        , _body(body)
        , _cannedError(cannedError)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::GetOperation);
        if (httpStatus > 0) {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, httpStatus);
        }
        (void) open(ReadOnly);
        if (!hold) {
            QTimer::singleShot(0, this, &TileReplyCannedReply::finishNow);
        }
    }

    void finishNow()
    {
        if (_cannedError != NoError) {
            setError(_cannedError, QStringLiteral("canned error"));
            emit errorOccurred(_cannedError);
        }
        setFinished(true);
        emit finished();
    }

    void abort() final {}

    qint64 bytesAvailable() const final { return (_body.size() - _offset) + QNetworkReply::bytesAvailable(); }

protected:
    qint64 readData(char* data, qint64 maxSize) final
    {
        const qint64 n = qMin<qint64>(maxSize, _body.size() - _offset);
        if (n <= 0) {
            return (_offset >= _body.size()) ? -1 : 0;
        }
        (void) memcpy(data, _body.constData() + _offset, static_cast<size_t>(n));
        _offset += n;
        return n;
    }

private:
    const QByteArray _body;
    const NetworkError _cannedError;
    qint64 _offset = 0;
};

/// Answers every request with a TileReplyCannedReply built from its public members
class TileReplyMockNam : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int httpStatus = 0;
    bool holdReplies = false;
    int requestCount = 0;
    QList<TileReplyCannedReply*> replies;

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        requestCount++;
        auto* const reply = new TileReplyCannedReply(request, body, error, httpStatus, holdReplies, this);
        replies.append(reply);
        return reply;
    }
};

inline QByteArray tileReplyPngBody()
{
    QImage image(32, 32, QImage::Format_RGB32);
    image.fill(Qt::darkGreen);
    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return QByteArray();
    }
    return data;
}

inline QByteArray tileReplyBingPlaceholder()
{
    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
}

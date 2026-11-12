#pragma once

#include <cstring>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

// Network stand-ins for tests of tile fetching: no request leaves the process.

/// Finishes with a canned body, error and HTTP status; a held reply stays open until finishNow()
class TileReplyCannedReply : public QNetworkReply
{
    Q_OBJECT

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

    void finishNow() { _finish(_cannedError); }

    /// Finishes a held reply with @a error instead of the canned one, and with @a httpStatus when that is given
    void failNow(NetworkError error, int httpStatus = 0)
    {
        if (httpStatus > 0) {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, httpStatus);
        }
        _finish(error);
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
    void _finish(NetworkError error)
    {
        if (error != NoError) {
            setError(error, QStringLiteral("canned error"));
            emit errorOccurred(error);
        }
        setFinished(true);
        emit finished();
    }

    const QByteArray _body;
    const NetworkError _cannedError;
    qint64 _offset = 0;
};

/// Answers every request with a TileReplyCannedReply built from its public members
class TileReplyMockNam : public QNetworkAccessManager
{
    Q_OBJECT

public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int httpStatus = 0;
    bool holdReplies = false;
    int requestCount = 0;
    QList<QNetworkRequest> requests;
    /// Null once the reply is deleted: the code under test deletes a reply that finished
    QList<QPointer<TileReplyCannedReply>> replies;

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        requestCount++;
        requests.append(request);
        auto* const reply = new TileReplyCannedReply(request, body, error, httpStatus, holdReplies, this);
        replies.append(reply);
        return reply;
    }
};

/// A PNG that decodes to an image of @a edgePixels squared and 4 bytes a pixel
inline QByteArray tileReplyPngBody(const QColor& color = Qt::darkGreen, int edgePixels = 32)
{
    QImage image(edgePixels, edgePixels, QImage::Format_RGB32);
    image.fill(color);
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

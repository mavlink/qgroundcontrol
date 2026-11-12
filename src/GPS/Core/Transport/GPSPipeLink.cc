#include "GPSPipeLink.h"

#include <utility>

#include "GPSPipe.h"

GPSPipeLink::GPSPipeLink(std::shared_ptr<GPSPipe> pipe, QObject* parent)
    : QObject(parent)
    , _pipe(std::move(pipe))
{
    const auto serve = [this]() {
        (void) QMetaObject::invokeMethod(this, &GPSPipeLink::_serveSession, Qt::QueuedConnection);
    };
    _pipe->setLinkNotifier(serve);
    // A session that asked before this object existed is served too.
    serve();
}

GPSPipeLink::~GPSPipeLink()
{
    // Waits for a notification that is running, so none posts to a destroyed object.
    _pipe->setLinkNotifier({});
    _pipe->close();
}

void GPSPipeLink::_serveSession()
{
    if (_sessionOver) {
        return;
    }
    if (_pipe->sessionClosed()) {
        _sessionOver = true;
        closeLink();
        deleteLater();
        return;
    }
    if (_pipe->openRequested() && !std::exchange(_openStarted, true)) {
        openLink();
    }
    if (const QByteArray output = _pipe->takeOutput(); !output.isEmpty()) {
        writeLink(output);
    }
}

void GPSPipeLink::linkOpened()
{
    _pipe->opened();
}

void GPSPipeLink::linkClosed(const QString& detail)
{
    _pipe->close(detail);
}

void GPSPipeLink::linkReceived(QByteArrayView bytes)
{
    _pipe->feed(bytes);
}

void GPSPipeLink::linkWrote(qsizetype bytes)
{
    _pipe->confirmWritten(bytes);
}

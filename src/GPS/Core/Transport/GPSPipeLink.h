#pragma once

#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QObject>
#include <QtCore/QString>

class GPSPipe;

/// The link's side of a GPSPipe for an owner with an event loop. It hears of what the session asks on its own thread,
/// whichever thread the session runs on, and passes on what the link reports. A subclass brings the link up and down
/// and sends the session's output.
///
/// Create one for each session, on the thread whose event loop serves the link. It deletes itself, with
/// deleteLater(), once the session is over and closeLink() has run. Destroyed before that, as with its parent, it
/// ends the link for the session.
class GPSPipeLink : public QObject
{
    Q_OBJECT

public:
    explicit GPSPipeLink(std::shared_ptr<GPSPipe> pipe, QObject* parent = nullptr);
    ~GPSPipeLink() override;

protected:
    /// The session waits for the link: bring it up and report linkOpened(), or linkClosed() when it cannot be.
    /// Called once.
    virtual void openLink() = 0;
    /// Sends @a bytes of the session's output, to be reported with linkWrote() once they are sent.
    virtual void writeLink(const QByteArray& bytes) = 0;
    /// The session is over: take the link down. Called once, also for a link that was never asked for.
    virtual void closeLink() = 0;

    void linkOpened();
    /// The link was lost, or could not be brought up, for the reason @a detail gives the user.
    void linkClosed(const QString& detail = {});
    void linkReceived(QByteArrayView bytes);
    void linkWrote(qsizetype bytes);

private:
    void _serveSession();

    const std::shared_ptr<GPSPipe> _pipe;
    bool _openStarted = false;
    bool _sessionOver = false;
};

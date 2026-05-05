#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>
#include <cstdint>

class OnboardLogEntry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_DISABLE_COPY_MOVE(OnboardLogEntry)

    Q_PROPERTY(uint id READ id CONSTANT FINAL)
    Q_PROPERTY(QDateTime time READ time NOTIFY timeChanged FINAL)
    Q_PROPERTY(uint size READ size NOTIFY sizeChanged FINAL)
    Q_PROPERTY(QString sizeStr READ sizeStr NOTIFY sizeChanged FINAL)
    Q_PROPERTY(bool received READ received NOTIFY receivedChanged FINAL)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged FINAL)
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)

public:
    enum class State : uint8_t
    {
        Pending,
        Available,
        Queued,
        Downloading,
        Downloaded,
        Erasing,
        Canceled,
        Skipped,
        Error,
    };
    Q_ENUM(State)

    explicit OnboardLogEntry(uint logId, const QDateTime& dateTime = QDateTime(), uint logSize = 0,
                             bool received = false, QObject* parent = nullptr);
    ~OnboardLogEntry() override;

    uint id() const { return _logID; }

    uint size() const { return _logSize; }

    QString sizeStr() const;

    QDateTime time() const { return _logTimeUTC; }

    bool received() const { return _received; }

    bool selected() const { return _selected; }

    State state() const { return _state; }

    QString status() const { return _status; }

    QString errorMessage() const { return _errorMessage; }

    QString ftpPath() const { return _ftpPath; }  // empty for LOG-protocol entries

    void setSize(uint size)
    {
        if (size != _logSize) {
            _logSize = size;
            emit sizeChanged();
        }
    }

    void setTime(const QDateTime& date)
    {
        if (date != _logTimeUTC) {
            _logTimeUTC = date;
            emit timeChanged();
        }
    }

    void setReceived(bool rec)
    {
        if (rec != _received) {
            _received = rec;
            emit receivedChanged();
        }
    }

    void setSelected(bool sel)
    {
        if (sel != _selected) {
            _selected = sel;
            emit selectedChanged();
        }
    }

    void setState(State state, const QString& status)
    {
        const bool stateDidChange = state != _state;
        const bool statusDidChange = status != _status;
        _state = state;
        _status = status;

        if (stateDidChange) {
            emit stateChanged();
        }
        if (statusDidChange) {
            emit statusChanged();
        }
    }

    void setStatus(const QString& status)
    {
        if (status != _status) {
            _status = status;
            emit statusChanged();
        }
    }

    void setErrorMessage(const QString& errorMessage)
    {
        if (errorMessage != _errorMessage) {
            _errorMessage = errorMessage;
            emit errorMessageChanged();
        }
    }

    void setFtpPath(const QString& path) { _ftpPath = path; }

signals:
    void timeChanged();
    void sizeChanged();
    void receivedChanged();
    void selectedChanged();
    void stateChanged();
    void statusChanged();
    void errorMessageChanged();

private:
    const uint _logID = 0;
    uint _logSize = 0;
    QDateTime _logTimeUTC;
    bool _received = false;
    bool _selected = false;
    State _state = State::Pending;
    QString _status;
    QString _errorMessage;
    QString _ftpPath;
};

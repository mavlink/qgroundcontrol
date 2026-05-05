#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class OnboardLogEntry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(uint id READ id NOTIFY idChanged)
    Q_PROPERTY(QDateTime time READ time NOTIFY timeChanged)
    Q_PROPERTY(uint size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString sizeStr READ sizeStr NOTIFY sizeChanged)
    Q_PROPERTY(bool received READ received NOTIFY receivedChanged)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit OnboardLogEntry(uint logId, const QDateTime& dateTime = QDateTime(), uint logSize = 0,
                                bool received = false, QObject* parent = nullptr);
    ~OnboardLogEntry();

    uint id() const { return _logID; }

    uint size() const { return _logSize; }

    QString sizeStr() const;

    QDateTime time() const { return _logTimeUTC; }

    bool received() const { return _received; }

    bool selected() const { return _selected; }

    QString status() const { return _status; }

    QString ftpPath() const { return _ftpPath; }  // empty for LOG-protocol entries

    void setId(uint id)
    {
        if (id != _logID) {
            _logID = id;
            emit idChanged();
        }
    }

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

    void setStatus(const QString& stat)
    {
        if (stat != _status) {
            _status = stat;
            emit statusChanged();
        }
    }

    void setFtpPath(const QString& path) { _ftpPath = path; }

signals:
    void idChanged();
    void timeChanged();
    void sizeChanged();
    void receivedChanged();
    void selectedChanged();
    void statusChanged();

private:
    uint _logID = 0;
    uint _logSize = 0;
    QDateTime _logTimeUTC;
    bool _received = false;
    bool _selected = false;
    QString _status = QStringLiteral("Pending");
    QString _ftpPath;
};

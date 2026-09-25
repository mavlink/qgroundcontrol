#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QtTypes>

/// Counts an owner's operations so work that calls out to observers can tell whether it is still
/// current afterwards: an observer may delete the owner or start a newer operation. The revision
/// must be a member of its owner.
class GPSRevision
{
public:
    GPSRevision() = default;
    Q_DISABLE_COPY_MOVE(GPSRevision)

    /// One operation's view of the revision, cheap to copy into callbacks.
    class Token
    {
    public:
        /// The owner still exists and no newer operation has started.
        bool isCurrent() const { return _owner && _revision->_value == _value; }

        quint64 value() const { return _value; }

    private:
        friend class GPSRevision;

        Token(const QObject* owner, const GPSRevision* revision)
            : _owner(owner)
            , _revision(revision)
            , _value(revision->_value)
        {}

        QPointer<const QObject> _owner;
        const GPSRevision* _revision;
        quint64 _value;
    };

    /// Joins the operation in progress.
    Token current(const QObject* owner) const { return Token(owner, this); }

    /// Starts a new operation, superseding every earlier token.
    Token advance(const QObject* owner)
    {
        invalidate();
        return current(owner);
    }

    /// Supersedes every earlier token.
    void invalidate() { ++_value; }

    quint64 value() const { return _value; }

private:
    quint64 _value = 0;
};

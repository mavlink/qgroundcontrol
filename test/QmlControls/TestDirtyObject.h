#pragma once

#include <QtCore/QObject>

// Shared test helper for QmlControls tests
class TestDirtyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

public:
    explicit TestDirtyObject(QObject* parent = nullptr) : QObject(parent) {}

    bool dirty() const { return _dirty; }
    void setDirty(bool dirty)
    {
        if (_dirty == dirty) {
            return;
        }

        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }

signals:
    void dirtyChanged(bool dirty);

private:
    bool _dirty = false;
};

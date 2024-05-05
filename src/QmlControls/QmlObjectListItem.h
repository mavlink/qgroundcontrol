#pragma once

#include <QtCore/QObject>

class QmlObjectListItem : public QObject
{
    Q_OBJECT

public:
    QmlObjectListItem(QObject* parent = nullptr);
    ~QmlObjectListItem();

    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

    virtual bool dirty(void) const { return _dirty; }
    virtual void setDirty(bool dirty);

signals:
    void dirtyChanged(bool dirty);

protected slots:
    void _setDirty(void);
    void _setIfDirty(bool dirty);

protected:
    bool _dirty = false;
    bool _ignoreDirtyChangeSignals = false;
};

/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtQmlIntegration/QtQmlIntegration>

/// Common base for QObject*-based item models (flat lists and trees).
/// Provides: dirty tracking, depth-counted begin/endResetModel, shared role constants, roleNames.
class ObjectItemModelBase : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ObjectItemModelBase(QObject* parent = nullptr);
    ~ObjectItemModelBase() override;

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

    bool dirty() const { return _dirty; }

    /// Depth-counted beginResetModel — only the outermost call has effect.
    void beginResetModel();

    /// Depth-counted endResetModel — only the outermost call has effect.
    void endResetModel();

    virtual int count() const = 0;
    virtual bool isEmpty() const { return (count() == 0); }
    virtual void setDirty(bool dirty) = 0;
    virtual void clear() = 0;

signals:
    void countChanged(int count);
    void dirtyChanged(bool dirty);

protected slots:
    void _childDirtyChanged(bool dirty);

protected:
    QHash<int, QByteArray> roleNames() const override;
    void _signalCountChangedIfNotNested();

    bool _dirty = false;
    uint _resetModelNestingCount = 0;

    static constexpr int ObjectRole = Qt::UserRole;
    static constexpr int TextRole = Qt::UserRole + 1;
};

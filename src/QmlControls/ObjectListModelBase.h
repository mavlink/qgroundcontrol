/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QAbstractListModel>
#include <QtQmlIntegration/QtQmlIntegration>


/// Base class for custom object list models: QmlObjectListModel, SparselObjectListModel
class ObjectListModelBase : public QAbstractListModel
{
    Q_OBJECT

public:
    ObjectListModelBase(QObject* parent = nullptr);
    ~ObjectListModelBase();

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /// Returns true if any of the items in the list are dirty. Requires each object to have
    /// a dirty property and dirtyChanged signal.
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

    bool dirty() const { return _dirty; }
    void beginResetModel(); ///< Supported nesting of calls such that only outermost call has effect
    void endResetModel(); ///< Supported nesting of calls such that only outermost call has effect

    virtual int count() const = 0;
    virtual bool isEmpty() const { return (count() == 0); }
    virtual void setDirty(bool dirty) = 0;
    virtual void clear() = 0;
    virtual void clearAndDeleteContents() = 0; ///< Clears the list and calls deleteLater on each entry
    virtual QObject* removeOne(const QObject* object) = 0;
    virtual bool contains(const QObject* object) = 0;

signals:
    void countChanged(int count);
    void dirtyChanged(bool dirtyChanged);

protected slots:
    void _childDirtyChanged(bool dirty);

protected:
    // Overrides from QAbstractListModel which must be implemented by derived classes
    int rowCount(const QModelIndex & parent = QModelIndex()) const override = 0;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override = 0;
    bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex()) override = 0;
    bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex()) override = 0;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override = 0;

    void _signalCountChangedIfNotNested();

    QHash<int, QByteArray> roleNames(void) const override;
    bool _dirty;
    bool _skipDirtyFirstItem;
    uint _resetModelNestingCount = 0;

    static constexpr int ObjectRole = Qt::UserRole;
    static constexpr int TextRole = Qt::UserRole + 1;
};

#pragma once

#include "ObjectListModelBase.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QmlObjectListModelLog)

class QmlObjectListModel : public ObjectListModelBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    QmlObjectListModel(QObject* parent = nullptr);

    // Overrides from ObjectListModelBase
    int count() const override final;
    bool isEmpty() const override final { return count() == 0; }
    void setDirty(bool dirty) override final;
    void clear() override final;
    QObject* removeOne(const QObject* object) override final { return removeAt(indexOf(object)); }
    bool contains(const QObject* object) override final { return _objectList.indexOf(object) != -1; }
    void clearAndDeleteContents() override final; ///< Clears the list and calls deleteLater on each entry

    // QmlObjectListModel specific methods
    Q_INVOKABLE QObject* get(int index);
    QObject* operator[](int index);
    const QObject* operator[](int index) const;
    void append(QObject* object); ///< Caller maintains responsibility for object ownership and deletion
    void append(QList<QObject*> objects); ///< Caller maintains responsibility for object ownership and deletion
    QObjectList swapObjectList(const QObjectList& newlist);
    QObject* removeAt(int index);
    void insert(int index, QObject* object);
    void insert(int index, QList<QObject*> objects);
    int indexOf(const QObject* object) { return _objectList.indexOf(object); }
    void move(int from, int to);
    template<class T> T value(int index) const { return qobject_cast<T>(_objectList[index]); }
    QList<QObject*>* objectList() { return &_objectList; }

protected:
    // Overrides from QAbstractListModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

private:
    QList<QObject*> _objectList;
};

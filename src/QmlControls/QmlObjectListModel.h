/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef QmlObjectListModel_H
#define QmlObjectListModel_H

#include <QAbstractListModel>

class QmlObjectListModel : public QAbstractListModel
{
    Q_OBJECT
    
public:
    QmlObjectListModel(QObject* parent = NULL);
    ~QmlObjectListModel();
    
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    
    /// Returns true if any of the items in the list are dirty. Requires each object to have
    /// a dirty property and dirtyChanged signal.
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

    Q_INVOKABLE QObject* get(int index)     { return _objectList[index]; }

    // Property accessors
    
    int count(void) const;
    
    bool dirty(void) { return _dirty; }
    void setDirty(bool dirty);
    
    void append(QObject* object);
    QObjectList swapObjectList(QObjectList newlist);
    void clear(void);
    QObject* removeAt(int i);
    QObject* removeOne(QObject* object) { return removeAt(indexOf(object)); }
    void insert(int i, QObject* object);
    QObject* operator[](int i);
    const QObject* operator[](int i) const;
    bool contains(QObject* object) { return _objectList.indexOf(object) != -1; }
    int indexOf(QObject* object) { return _objectList.indexOf(object); }
    template<class T> T value(int index) { return qobject_cast<T>(_objectList[index]); }

    /// Calls deleteLater on all items and this itself.
    void deleteListAndContents(void);

    /// Clears the list and calls delete on each entry
    void clearAndDeleteContents(void);

signals:
    void countChanged(int count);
    void dirtyChanged(bool dirtyChanged);
    
private slots:
    void _childDirtyChanged(bool dirty);
    
private:
    // Overrides from QAbstractListModel
    virtual int	rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual QHash<int, QByteArray> roleNames(void) const;
    virtual bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    virtual bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	
private:
    QList<QObject*> _objectList;
    
    bool _dirty;
    bool _skipDirtyFirstItem;
        
    static const int ObjectRole;
    static const int TextRole;
};

#endif

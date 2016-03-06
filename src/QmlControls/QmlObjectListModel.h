/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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

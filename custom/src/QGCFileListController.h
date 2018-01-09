/*!
 *   @brief QML Simple File List Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QVector>
#include <QQmlListProperty>

class QGCFileListController;

//-----------------------------------------------------------------------------
// File List Item
class QGCFileListItem : public QObject
{
    Q_OBJECT
public:
    QGCFileListItem()
        : _parent(NULL)
        , _selected(false)
    {
    }
    QGCFileListItem(QGCFileListController* parent, QString fileName, quint64 size)
        : QObject((QObject*)parent)
        , _parent(parent)
        , _fileName(fileName)
        , _selected(false)
        , _size(size)
    {
    }

    Q_PROPERTY(QString  fileName    READ fileName                       CONSTANT)
    Q_PROPERTY(bool     selected    READ selected   WRITE setSelected   NOTIFY selectedChanged)
    Q_PROPERTY(quint64  size        READ size                           CONSTANT)
    Q_PROPERTY(QString  sizeStr     READ sizeStr                        CONSTANT)

    QString     fileName    () { return _fileName; }
    bool        selected    () { return _selected; }
    quint64     size        () { return _size; }
    QString     sizeStr     ();

    void        setSelected (bool sel);

signals:
    void        selectedChanged ();

protected:
    QGCFileListController*      _parent;
    QString                     _fileName;
    bool                        _selected;
    quint64                     _size;
};

//-----------------------------------------------------------------------------
class QGCFileListController : public QObject
{
    Q_OBJECT
    friend class QGCFileListItem;
public:
    QGCFileListController              (QObject* parent);
    ~QGCFileListController             () {}

    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
    Q_PROPERTY(QQmlListProperty<QGCFileListItem> fileList READ fileList NOTIFY fileListChanged)

    Q_INVOKABLE void selectAllFiles     (bool selected);

    QQmlListProperty<QGCFileListItem>   fileList    ();
    QVector<QGCFileListItem*>&          fileListV   () { return _fileList; }

    void        appendFileItem          (QGCFileListItem* fileItem);
    QGCFileListItem* fileItem           (int index);
    int         fileCount               ();
    void        clearFileItems          ();
    int         selectedCount           () { return _selectedCount; }

signals:
    void        selectedCountChanged    ();
    void        fileListChanged         ();

private:
    static void appendFileItem(QQmlListProperty<QGCFileListItem>*, QGCFileListItem*);
    static QGCFileListItem* fileItem(QQmlListProperty<QGCFileListItem>*, int);
    static int fileCount(QQmlListProperty<QGCFileListItem>*);
    static void clearFileItems(QQmlListProperty<QGCFileListItem>*);

private:
    int                         _selectedCount;
    QVector<QGCFileListItem*>   _fileList;
};

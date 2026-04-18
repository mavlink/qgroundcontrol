#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QGCLoggingCategoryItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)

public:
    QGCLoggingCategoryItem(const QString& shortCategory_, const QString& fullCategory_, int logLevel_,
                           QObject* parent = nullptr);

    bool enabled() const { return _logLevel <= QtDebugMsg; }

    void setEnabled(bool enabled);

    int logLevel() const { return _logLevel; }

    void setLogLevel(int level);
    void setLogLevelFromManager(int level);

    QString shortCategory;
    QString fullCategory;

signals:
    void logLevelChanged();
    void enabledChanged();

private:
    int _logLevel = QtWarningMsg;
    bool _updatingFromManager = false;
};

class LoggingCategoryFlatModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Roles
    {
        ShortNameRole = Qt::UserRole + 1,
        FullNameRole,
        EnabledRole,
        LogLevelRole,
    };
    Q_ENUM(Roles)

    explicit LoggingCategoryFlatModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    void insertSorted(QGCLoggingCategoryItem* item);
    QGCLoggingCategoryItem* findByFullName(const QString& fullName) const;

    int count() const { return _items.count(); }

    QGCLoggingCategoryItem* at(int i) const { return _items.at(i); }

private:
    QList<QGCLoggingCategoryItem*> _items;
};

// ---------------------------------------------------------------------------
// Tree model (proper QAbstractItemModel for TreeView)
// ---------------------------------------------------------------------------

struct LoggingCategoryTreeNode
{
    QGCLoggingCategoryItem* item = nullptr;
    LoggingCategoryTreeNode* parent = nullptr;
    QList<LoggingCategoryTreeNode*> children;
    bool expanded = false;

    ~LoggingCategoryTreeNode() { qDeleteAll(children); }
};

class LoggingCategoryTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Roles
    {
        ShortNameRole = Qt::UserRole + 1,
        FullNameRole,
        EnabledRole,
        LogLevelRole,
    };
    Q_ENUM(Roles)

    explicit LoggingCategoryTreeModel(QObject* parent = nullptr);
    ~LoggingCategoryTreeModel() override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

    void insertCategory(const QStringList& pathSegments, const QString& fullCategory, QGCLoggingCategoryItem* item);

private:
    LoggingCategoryTreeNode* nodeFromIndex(const QModelIndex& index) const;
    LoggingCategoryTreeNode* findOrCreateIntermediateNode(LoggingCategoryTreeNode* parent, const QString& segment,
                                                          const QString& fullPrefix);
    int insertionIndex(LoggingCategoryTreeNode* parent, const QString& name) const;

    LoggingCategoryTreeNode _root;
};

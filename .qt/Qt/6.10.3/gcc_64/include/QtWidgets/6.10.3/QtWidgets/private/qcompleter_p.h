// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCOMPLETER_P_H
#define QCOMPLETER_P_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qobject_p.h"

#include "QtWidgets/qabstractitemview.h"
#include "QtCore/qabstractproxymodel.h"
#include "QtCore/qmap.h"
#include "qcompleter.h"
#include "qstyleditemdelegate.h"
#include "QtGui/qpainter.h"

#include "private/qabstractproxymodel_p.h"
#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(completer);

QT_BEGIN_NAMESPACE

class QCompletionModel;

class QCompleterPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QCompleter)

public:
    QCompleterPrivate();
    ~QCompleterPrivate() { delete popup; }
    void init(QAbstractItemModel *model = nullptr);

    QPointer<QWidget> widget;
    QCompletionModel *proxy;
    QAbstractItemView *popup;
    QCompleter::CompletionMode mode;
    Qt::MatchFlags filterMode;

    QString prefix;
    Qt::CaseSensitivity cs;
    int role;
    int column;
    int maxVisibleItems;
    QCompleter::ModelSorting sorting;
    bool wrap;

    bool eatFocusOut;
    QRect popupRect;
    bool hiddenBecauseNoMatch;

    void showPopup(const QRect&);
    void _q_complete(QModelIndex, bool = false);
    void _q_completionSelected(const QItemSelection&);
    void _q_autoResizePopup();
    void _q_fileSystemModelDirectoryLoaded(const QString &path);
    void setCurrentIndex(QModelIndex, bool = true);

    static QCompleterPrivate *get(QCompleter *o) { return o->d_func(); }
    static const QCompleterPrivate *get(const QCompleter *o) { return o->d_func(); }
};

class QIndexMapper
{
public:
    QIndexMapper() : v(false), f(0), t(-1) { }
    QIndexMapper(int f, int t) : v(false), f(f), t(t) { }
    QIndexMapper(const QList<int> &vec) : v(true), vector(vec), f(-1), t(-1) { }

    inline int count() const { return v ? vector.size() : t - f + 1; }
    inline int operator[] (int index) const { return v ? vector[index] : f + index; }
    inline int indexOf(int x) const { return v ? vector.indexOf(x) : ((t < f) ? -1 : x - f); }
    inline bool isValid() const { return !isEmpty(); }
    inline bool isEmpty() const { return v ? vector.isEmpty() : (t < f); }
    inline void append(int x) { Q_ASSERT(v); vector.append(x); }
    inline int first() const { return v ? vector.first() : f; }
    inline int last() const { return v ? vector.last() : t; }
    inline int from() const { Q_ASSERT(!v); return f; }
    inline int to() const { Q_ASSERT(!v); return t; }
    inline int cost() const { return vector.size()+2; }

private:
    bool v;
    QList<int> vector;
    int f, t;
};

struct QMatchData {
    QMatchData() : exactMatchIndex(-1), partial(false) { }
    QMatchData(const QIndexMapper& indices, int em, bool p) :
        indices(indices), exactMatchIndex(em), partial(p) { }
    QIndexMapper indices;
    inline bool isValid() const { return indices.isValid(); }
    int  exactMatchIndex;
    bool partial;
};

class QCompletionEngine
{
public:
    typedef QMap<QString, QMatchData> CacheItem;
    typedef QMap<QModelIndex, CacheItem> Cache;

    QCompletionEngine(QCompleterPrivate *c) : c(c), curRow(-1), cost(0) { }
    virtual ~QCompletionEngine() { }

    void filter(const QStringList &parts);

    QMatchData filterHistory();
    bool matchHint(const QString &part, const QModelIndex &parent, QMatchData *m) const;

    void saveInCache(QString, const QModelIndex&, const QMatchData&);
    bool lookupCache(const QString &part, const QModelIndex &parent, QMatchData *m) const;

    virtual void filterOnDemand(int) { }
    virtual QMatchData filter(const QString&, const QModelIndex&, int) = 0;

    int matchCount() const { return curMatch.indices.count() + historyMatch.indices.count(); }

    QMatchData curMatch, historyMatch;
    QCompleterPrivate *c;
    QStringList curParts;
    QModelIndex curParent;
    int curRow;

    Cache cache;
    int cost;
};

class QSortedModelEngine : public QCompletionEngine
{
public:
    QSortedModelEngine(QCompleterPrivate *c) : QCompletionEngine(c) { }
    QMatchData filter(const QString&, const QModelIndex&, int) override;
    QIndexMapper indexHint(QString, const QModelIndex&, Qt::SortOrder);
    Qt::SortOrder sortOrder(const QModelIndex&) const;
};

class QUnsortedModelEngine : public QCompletionEngine
{
public:
    QUnsortedModelEngine(QCompleterPrivate *c) : QCompletionEngine(c) { }

    void filterOnDemand(int) override;
    QMatchData filter(const QString&, const QModelIndex&, int) override;
private:
    int buildIndices(const QString& str, const QModelIndex& parent, int n,
                     const QIndexMapper& iv, QMatchData* m);
};

class QCompleterItemDelegate : public QStyledItemDelegate
{
public:
    QCompleterItemDelegate(QAbstractItemView *view)
        : QStyledItemDelegate(view), view(view) { }
    void paint(QPainter *p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const override {
        QStyleOptionViewItem optCopy = opt;
        optCopy.showDecorationSelected = true;
        if (view->currentIndex() == idx)
            optCopy.state |= QStyle::State_HasFocus;
        QStyledItemDelegate::paint(p, optCopy, idx);
    }

private:
    QAbstractItemView *view;
};

class QCompletionModelPrivate;

class QCompletionModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    QCompletionModel(QCompleterPrivate *c, QObject *parent);

    void createEngine();
    void setFiltered(bool);
    void filter(const QStringList& parts);
    int completionCount() const;
    int currentRow() const { return engine->curRow; }
    bool setCurrentRow(int row);
    QModelIndex currentIndex(bool) const;

    QModelIndex index(int row, int column, const QModelIndex & = QModelIndex()) const override;
    int rowCount(const QModelIndex &index = QModelIndex()) const override;
    int columnCount(const QModelIndex &index = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex & = QModelIndex()) const override { return QModelIndex(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    QCompleterPrivate *c;
    QScopedPointer<QCompletionEngine> engine;
    bool showAll;

    Q_DECLARE_PRIVATE(QCompletionModel)

signals:
    void rowsAdded();

public Q_SLOTS:
    void invalidate();
    void rowsInserted();
    void modelDestroyed();
};

class QCompletionModelPrivate : public QAbstractProxyModelPrivate
{
    Q_DECLARE_PUBLIC(QCompletionModel)
};

QT_END_NAMESPACE

#endif // QCOMPLETER_P_H

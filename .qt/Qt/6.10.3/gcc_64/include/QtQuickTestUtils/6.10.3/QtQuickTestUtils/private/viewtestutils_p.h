// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICKVIEWTESTUTILS_P_H
#define QQUICKVIEWTESTUTILS_P_H

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

#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlExpression>
#include <QtQuick/QQuickItem>
#include <QtCore/private/qglobal_p.h>
#include <QtQuick/private/qtquickglobal_p.h>

QT_FORWARD_DECLARE_CLASS(QQuickView)
QT_FORWARD_DECLARE_CLASS(QQuickItemViewPrivate)
QT_FORWARD_DECLARE_CLASS(FxViewItem)
QT_FORWARD_DECLARE_CLASS(QPointingDevice)

QT_BEGIN_NAMESPACE

namespace QQuickViewTestUtils
{
    QQuickView *createView();

    void centerOnScreen(QQuickWindow *window, const QSize &size);
    void centerOnScreen(QQuickWindow *window);
    void moveMouseAway(QQuickView *window);

    QList<int> adjustIndexesForAddDisplaced(const QList<int> &indexes, int index, int count);
    QList<int> adjustIndexesForMove(const QList<int> &indexes, int from, int to, int count);
    QList<int> adjustIndexesForRemoveDisplaced(const QList<int> &indexes, int index, int count);

    struct ListChange {
        enum { Inserted, Removed, Moved, SetCurrent, SetContentY, Polish } type;
        int index;
        int count;
        int to;     // Move
        qreal pos;  // setContentY

        static ListChange insert(int index, int count = 1) { ListChange c = { Inserted, index, count, -1, 0.0 }; return c; }
        static ListChange remove(int index, int count = 1) { ListChange c = { Removed, index, count, -1, 0.0 }; return c; }
        static ListChange move(int index, int to, int count) { ListChange c = { Moved, index, count, to, 0.0 }; return c; }
        static ListChange setCurrent(int index) { ListChange c = { SetCurrent, index, -1, -1, 0.0 }; return c; }
        static ListChange setContentY(qreal pos) { ListChange c = { SetContentY, -1, -1, -1, pos }; return c; }
        static ListChange polish() { ListChange c = { Polish, -1, -1, -1, 0.0 }; return c; }
    };

    class QaimModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        enum Roles { Name = Qt::UserRole+1, Number = Qt::UserRole+2 };

        QaimModel(QObject *parent=0);

        int rowCount(const QModelIndex &parent=QModelIndex()) const override;
        int columnCount(const QModelIndex &parent=QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const override;
        QHash<int,QByteArray> roleNames() const override;

        int count() const;
        QString name(int index) const;
        QString number(int index) const;

        Q_INVOKABLE void addItem(const QString &name, const QString &number);
        void addItems(const QList<std::pair<QString, QString> > &items);
        void insertItem(int index, const QString &name, const QString &number);
        void insertItems(int index, const QList<std::pair<QString, QString> > &items);

        Q_INVOKABLE void removeItem(int index);
        void removeItems(int index, int count);

        void moveItem(int from, int to);
        void moveItems(int from, int to, int count);

        void modifyItem(int idx, const QString &name, const QString &number);

        void clear();
        void reset();
        void resetItems(const QList<std::pair<QString, QString> > &items);

        void matchAgainst(const QList<std::pair<QString, QString> > &other, const QString &error1, const QString &error2);

        using QAbstractListModel::dataChanged;

        int columns = 1;

    private:
        QList<std::pair<QString,QString> > list;
    };

    class ListRange
    {
    public:
        ListRange();
        ListRange(const ListRange &other);
        ListRange(int start, int end);

        ~ListRange();

        ListRange operator+(const ListRange &other) const;
        bool operator==(const ListRange &other) const;
        bool operator!=(const ListRange &other) const;

        bool isValid() const;
        int count() const;

        QList<std::pair<QString,QString> > getModelDataValues(const QaimModel &model);

        QList<int> indexes;
        bool valid;
    };

    template<typename T>
    static void qquickmodelviewstestutil_move(int from, int to, int n, T *items)
    {
        if (from > to) {
            // Only move forwards - flip if backwards moving
            int tfrom = from;
            int tto = to;
            from = tto;
            to = tto+n;
            n = tfrom-tto;
        }

        T replaced;
        int i=0;
        typename T::ConstIterator it=items->begin(); it += from+n;
        for (; i<to-from; ++i,++it)
            replaced.append(*it);
        i=0;
        it=items->begin(); it += from;
        for (; i<n; ++i,++it)
            replaced.append(*it);
        typename T::ConstIterator f=replaced.begin();
        typename T::Iterator t=items->begin(); t += from;
        for (; f != replaced.end(); ++f, ++t)
            *t = *f;
    }

    class StressTestModel : public QAbstractListModel
    {
        Q_OBJECT

    public:

        StressTestModel();

        int rowCount(const QModelIndex &) const override;
        QVariant data(const QModelIndex &, int) const override;

    public Q_SLOTS:
        void updateModel();

    private:
        int m_rowCount;
    };

#if QT_CONFIG(quick_itemview) && defined(QT_BUILD_INTERNAL)
    [[nodiscard]] bool testVisibleItems(const QQuickItemViewPrivate *priv,
        bool *nonUnique, FxViewItem **failItem, int *expectedIdx);
#endif
}

namespace QQuickTouchUtils {
    void flush(QQuickWindow *window);
}

namespace QQuickTest {
    [[nodiscard]] bool initView(QQuickView &v, const QUrl &url,
        bool moveMouseOut = true, QByteArray *errorMessage = nullptr);
    [[nodiscard]] bool showView(QQuickView &v, const QUrl &url);

    void pointerPress(const QPointingDevice *dev, QQuickWindow *window,
                      int pointId, const QPoint &p, Qt::MouseButton button = Qt::LeftButton,
                      Qt::KeyboardModifiers modifiers = Qt::NoModifier, int delay = -1);

    void pointerMove(const QPointingDevice *dev, QQuickWindow *window, int pointId,
                     const QPoint &p, int delay = -1);

    void pointerRelease(const QPointingDevice *dev, QQuickWindow *window, int pointId,
                        const QPoint &p, Qt::MouseButton button = Qt::LeftButton,
                        Qt::KeyboardModifiers modifiers = Qt::NoModifier, int delay = -1);

    void pointerMoveAndPress(const QPointingDevice *dev, QQuickWindow *window,
                             int pointId, const QPoint &p, Qt::MouseButton button = Qt::LeftButton,
                             Qt::KeyboardModifiers modifiers = Qt::NoModifier, int delay = -1);

    void pointerMoveAndRelease(const QPointingDevice *dev, QQuickWindow *window,
                               int pointId, const QPoint &p, Qt::MouseButton button = Qt::LeftButton,
                               Qt::KeyboardModifiers modifiers = Qt::NoModifier, int delay = -1);

    void pointerFlick(const QPointingDevice *dev, QQuickWindow *window,
                      int pointId, const QPoint &from, const QPoint &to, int duration,
                      Qt::MouseButton button = Qt::LeftButton,
                      Qt::KeyboardModifiers modifiers = Qt::NoModifier, int delay = -1);
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickViewTestUtils::QaimModel*)
Q_DECLARE_METATYPE(QQuickViewTestUtils::ListChange)
Q_DECLARE_METATYPE(QList<QQuickViewTestUtils::ListChange>)
Q_DECLARE_METATYPE(QQuickViewTestUtils::ListRange)


#endif // QQUICKVIEWTESTUTILS_P_H

// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICKVISUALTESTUTILS_P_H
#define QQUICKVISUALTESTUTILS_P_H

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

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtQml/qqmlexpression.h>
#include <QtQuick/private/qquickitem_p.h>

#include <private/qmlutils_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickItemView;
class QQuickWindow;

namespace QQuickVisualTestUtils
{
    QQuickItem *findVisibleChild(QQuickItem *parent, const QString &objectName);

    void dumpTree(QQuickItem *parent, int depth = 0);

    void moveMouseAway(QQuickWindow *window);
    void centerOnScreen(QQuickWindow *window);

    template<typename F>
    void forEachStep(int steps, F &&func)
    {
        if (steps == 1) {
            // that's odd usage, but cut to the chase then
            func(qreal(1));
            return;
        }

        for (int i = 0; i < steps; ++i) {
            // - 1 because that gives us {0, 0.5, 1} for progress (if steps == 3),
            // rather than {0, 0.33, 0.66}.
            const qreal progress = qreal(i) / (steps - 1);
            func(progress);
        }
    }

    [[nodiscard]] QPoint lerpPoints(const QPoint &point1, const QPoint &point2, qreal t);

    class [[nodiscard]] PointLerper
    {
    public:
        PointLerper(QQuickWindow *window,
            const QPoint &startingPosition = QPoint(0, 0),
            const QPointingDevice *pointingDevice = QPointingDevice::primaryPointingDevice());

        void move(const QPoint &pos, int steps = 10, int delayInMilliseconds = 1);
        void move(int x, int y, int steps = 10, int delayInMilliseconds = 1);

    private:
        QQuickWindow *mWindow = nullptr;
        const QPointingDevice *mPointingDevice = nullptr;
        QPoint mFrom;
    };

    [[nodiscard]] bool isDelegateVisible(QQuickItem *item);

    /*
       Find an item with the specified objectName.  If index is supplied then the
       item must also evaluate the {index} expression equal to index
    */
    template<typename T>
    T *findItem(QQuickItem *parent, const QString &objectName, int index = -1)
    {
        using namespace Qt::StringLiterals;

        const QMetaObject &mo = T::staticMetaObject;
        for (int i = 0; i < parent->childItems().size(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
            if (!item)
                continue;
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
                if (index != -1) {
                    QQmlContext *context = qmlContext(item);
                    if (!context->isValid())
                        continue;
                    QQmlExpression e(context, item, u"index"_s);
                    if (e.evaluate().toInt() == index)
                        return static_cast<T*>(item);
                } else {
                    return static_cast<T*>(item);
                }
            }
            item = findItem<T>(item, objectName, index);
            if (item)
                return static_cast<T*>(item);
        }

        return 0;
    }

    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName, bool visibleOnly = true)
    {
        QList<T*> items;
        const QMetaObject &mo = T::staticMetaObject;
        for (int i = 0; i < parent->childItems().size(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
            if (!item || (visibleOnly && (!item->isVisible() || QQuickItemPrivate::get(item)->culled)))
                continue;
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
                items.append(static_cast<T*>(item));
            items += findItems<T>(item, objectName);
        }

        return items;
    }

    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName, const QList<int> &indexes)
    {
        QList<T*> items;
        for (int i=0; i<indexes.size(); i++)
            items << qobject_cast<QQuickItem*>(findItem<T>(parent, objectName, indexes[i]));
        return items;
    }

    bool compareImages(const QImage &ia, const QImage &ib, QString *errorMessage);

    struct SignalMultiSpy : public QObject
    {
        Q_OBJECT
    public:
        QList<QObject *> senders;
        QList<QByteArray> signalNames;

        template <typename Func1>
        QMetaObject::Connection connectToSignal(const typename QtPrivate::FunctionPointer<Func1>::Object *obj, Func1 signal,
                                                              Qt::ConnectionType type = Qt::AutoConnection)
        {
            return connect(obj, signal, this, &SignalMultiSpy::receive, type);
        }

        void clear() {
            senders.clear();
            signalNames.clear();
        }

    public Q_SLOTS:
        void receive() {
            QMetaMethod m = sender()->metaObject()->method(senderSignalIndex());
            senders << sender();
            signalNames << m.name();
        }
    };

    enum class FindViewDelegateItemFlag {
        None = 0x0,
        PositionViewAtIndex = 0x01
    };
    Q_DECLARE_FLAGS(FindViewDelegateItemFlags, FindViewDelegateItemFlag)

#if QT_CONFIG(quick_itemview)
    QQuickItem* findViewDelegateItem(QQuickItemView *itemView, int index,
        FindViewDelegateItemFlags flags = FindViewDelegateItemFlag::PositionViewAtIndex);
#endif

    /*!
        \internal

        Same as above except allows use in QTRY_* functions without having to call it again
        afterwards to assign the delegate.
    */
    template<typename T>
    [[nodiscard]] bool findViewDelegateItem(QQuickItemView *itemView, int index, T &delegateItem,
        FindViewDelegateItemFlags flags = FindViewDelegateItemFlag::PositionViewAtIndex)
    {
        delegateItem = qobject_cast<T>(findViewDelegateItem(itemView, index, flags));
        return delegateItem != nullptr;
    }

    class QQuickApplicationHelper
    {
    public:
        QQuickApplicationHelper(QQmlDataTest *testCase, const QString &testFilePath,
                const QVariantMap &initialProperties = {},
                const QStringList &qmlImportPaths = {});

        // Return a C-style string instead of QString because that's what QTest uses for error messages,
        // so it saves code at the calling site.
        inline const char *failureMessage() const
        {
            return errorMessage.constData();
        }

        QQmlEngine engine;
        QScopedPointer<QObject> cleanup;
        QQuickWindow *window = nullptr;

        bool ready = false;
        // Store as a byte array so that we can return its raw data safely;
        // using qPrintable() in failureMessage() will construct a throwaway QByteArray
        // that is destroyed before the function returns.
        QByteArray errorMessage;
    };

    class MnemonicKeySimulator
    {
        Q_DISABLE_COPY(MnemonicKeySimulator)
    public:
        explicit MnemonicKeySimulator(QWindow *window);

        void press(Qt::Key key);
        void release(Qt::Key key);
        void click(Qt::Key key);

    private:
        QPointer<QWindow> m_window;
        Qt::KeyboardModifiers m_modifiers;
    };

    QPoint mapCenterToWindow(const QQuickItem *item);
    QPoint mapToWindow(const QQuickItem *item, qreal relativeX, qreal relativeY);
    QPoint mapToWindow(const QQuickItem *item, const QPointF &relativePos);
}

#define SKIP_IF_NO_WINDOW_ACTIVATION \
do { \
    if (!(QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))) \
        QSKIP("Window activation is not supported on this platform"); \
} while (false)

#define SKIP_IF_NO_WINDOW_GRAB \
do { \
    if (QGuiApplication::platformName() == QLatin1String("minimal")) \
        QSKIP("grabWindow is not supported on the minimal platform"); \
} while (false)

#define SKIP_IF_NO_MOUSE_HOVER \
do { \
    if ((QGuiApplication::platformName() == QLatin1String("offscreen")) \
            || (QGuiApplication::platformName() == QLatin1String("minimal"))) \
        QSKIP("Mouse hovering is not supported on the offscreen/minimal platforms"); \
} while (false)

QT_END_NAMESPACE

#endif // QQUICKVISUALTESTUTILS_P_H

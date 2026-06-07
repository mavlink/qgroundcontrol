// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSCREEN_P_H
#define QQUICKSCREEN_P_H

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

#include <QtQml/qqml.h>
#include <QtQuick/private/qtquickglobal_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE


class QQuickItem;
class QQuickWindow;
class QScreen;


class Q_QUICK_EXPORT QQuickScreenInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString manufacturer READ manufacturer NOTIFY manufacturerChanged REVISION(2, 10) FINAL)
    Q_PROPERTY(QString model READ model NOTIFY modelChanged REVISION(2, 10) FINAL)
    Q_PROPERTY(QString serialNumber READ serialNumber NOTIFY serialNumberChanged REVISION(2, 10) FINAL)
    Q_PROPERTY(int width READ width NOTIFY widthChanged FINAL)
    Q_PROPERTY(int height READ height NOTIFY heightChanged FINAL)
    Q_PROPERTY(int desktopAvailableWidth READ desktopAvailableWidth NOTIFY desktopGeometryChanged FINAL)
    Q_PROPERTY(int desktopAvailableHeight READ desktopAvailableHeight NOTIFY desktopGeometryChanged FINAL)
    Q_PROPERTY(qreal logicalPixelDensity READ logicalPixelDensity NOTIFY logicalPixelDensityChanged FINAL)
    Q_PROPERTY(qreal pixelDensity READ pixelDensity NOTIFY pixelDensityChanged FINAL)
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged FINAL)
    Q_PROPERTY(Qt::ScreenOrientation primaryOrientation READ primaryOrientation NOTIFY primaryOrientationChanged FINAL)
    Q_PROPERTY(Qt::ScreenOrientation orientation READ orientation NOTIFY orientationChanged FINAL)

    Q_PROPERTY(int virtualX READ virtualX NOTIFY virtualXChanged REVISION(2, 3) FINAL)
    Q_PROPERTY(int virtualY READ virtualY NOTIFY virtualYChanged REVISION(2, 3) FINAL)
    QML_NAMED_ELEMENT(ScreenInfo)
    QML_ADDED_IN_VERSION(2, 3)
    QML_UNCREATABLE("ScreenInfo can only be used via the attached property.")

public:
    QQuickScreenInfo(QObject *parent = nullptr, QScreen *wrappedScreen = nullptr);

    QString name() const;
    QString manufacturer() const;
    QString model() const;
    QString serialNumber() const;
    int width() const;
    int height() const;
    int desktopAvailableWidth() const;
    int desktopAvailableHeight() const;
    qreal logicalPixelDensity() const;
    qreal pixelDensity() const;
    qreal devicePixelRatio() const;
    Qt::ScreenOrientation primaryOrientation() const;
    Qt::ScreenOrientation orientation() const;
    int virtualX() const;
    int virtualY() const;

    void setWrappedScreen(QScreen *screen);
    QScreen *wrappedScreen() const;

Q_SIGNALS:
    void nameChanged();
    Q_REVISION(2, 10) void manufacturerChanged();
    Q_REVISION(2, 10) void modelChanged();
    Q_REVISION(2, 10) void serialNumberChanged();
    void widthChanged();
    void heightChanged();
    void desktopGeometryChanged();
    void logicalPixelDensityChanged();
    void pixelDensityChanged();
    void devicePixelRatioChanged();
    void primaryOrientationChanged();
    void orientationChanged();
    Q_REVISION(2, 3) void virtualXChanged();
    Q_REVISION(2, 3) void virtualYChanged();

protected:
    QPointer<QScreen> m_screen;
};

class Q_QUICK_EXPORT QQuickScreenAttached : public QQuickScreenInfo
{
    Q_OBJECT

    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickScreenAttached(QObject* attachee);

    //Treats int as Qt::ScreenOrientation, due to QTBUG-20639
    Q_INVOKABLE int angleBetween(int a, int b);

    void windowChanged(QQuickWindow*);

protected Q_SLOTS:
    void screenChanged(QScreen*);

private:
    QQuickWindow* m_window = nullptr;
    QQuickItem* m_attachee;
};

class Q_QUICK_EXPORT QQuickScreen : public QObject
{
    Q_OBJECT
    QML_ATTACHED(QQuickScreenAttached)
    QML_NAMED_ELEMENT(Screen)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Screen can only be used via the attached property.")

public:
    static QQuickScreenAttached *qmlAttachedProperties(QObject *object){ return new QQuickScreenAttached(object); }
};

QT_END_NAMESPACE

#endif

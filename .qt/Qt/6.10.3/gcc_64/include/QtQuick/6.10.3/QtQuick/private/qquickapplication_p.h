// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKAPPLICATION_P_H
#define QQUICKAPPLICATION_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/private/qquickscreen_p.h>

#include <QtQml/qqml.h>
#include <QtQml/private/qqmlglobal_p.h>

#include <QtGui/qfont.h>
#include <QtGui/qstylehints.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickApplication : public QQmlApplication
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged FINAL) // deprecated, use 'state' instead
    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection NOTIFY layoutDirectionChanged FINAL)
    Q_PROPERTY(bool supportsMultipleWindows READ supportsMultipleWindows CONSTANT FINAL)
    Q_PROPERTY(Qt::ApplicationState state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(QFont font READ font CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickScreenInfo> screens READ screens NOTIFY screensChanged FINAL)
    Q_PROPERTY(QStyleHints *styleHints READ styleHints CONSTANT FINAL)

    QML_NAMED_ELEMENT(Application)
    QML_SINGLETON
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickApplication(QObject *parent = nullptr);
    virtual ~QQuickApplication();
    bool active() const;
    Qt::LayoutDirection layoutDirection() const;
    bool supportsMultipleWindows() const;
    Qt::ApplicationState state() const;
    QFont font() const;
    QQmlListProperty<QQuickScreenInfo> screens();
    QString displayName() const;
    void setDisplayName(const QString &displayName);
    QStyleHints *styleHints();

Q_SIGNALS:
    void activeChanged();
    void displayNameChanged();
    void layoutDirectionChanged();
    void stateChanged(Qt::ApplicationState state);
    void screensChanged();

private Q_SLOTS:
    void updateScreens();

private:
    Q_DISABLE_COPY(QQuickApplication)
    std::vector<std::unique_ptr<QQuickScreenInfo>> m_screens;
};

QT_END_NAMESPACE

#endif // QQUICKAPPLICATION_P_H

// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSHORTCUT_P_H
#define QQUICKSHORTCUT_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qvariant.h>
#include <QtGui/qkeysequence.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>
#include <QtQuick/private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QShortcutEvent;

class Q_QUICK_EXPORT QQuickShortcut : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QVariant sequence READ sequence WRITE setSequence NOTIFY sequenceChanged FINAL)
    Q_PROPERTY(QVariantList sequences READ sequences WRITE setSequences NOTIFY sequencesChanged FINAL REVISION(2, 9))
    Q_PROPERTY(QString nativeText READ nativeText NOTIFY nativeTextChanged FINAL REVISION(2, 6))
    Q_PROPERTY(QString portableText READ portableText NOTIFY portableTextChanged FINAL REVISION(2, 6))
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat NOTIFY autoRepeatChanged FINAL)
    Q_PROPERTY(Qt::ShortcutContext context READ context WRITE setContext NOTIFY contextChanged FINAL)
    QML_NAMED_ELEMENT(Shortcut)
    QML_ADDED_IN_VERSION(2, 5)

public:
    explicit QQuickShortcut(QObject *parent = nullptr);
    ~QQuickShortcut();

    QVariant sequence() const;
    void setSequence(const QVariant &sequence);

    QVariantList sequences() const;
    void setSequences(const QVariantList &sequences);

    QString nativeText() const;
    QString portableText() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool autoRepeat() const;
    void setAutoRepeat(bool repeat);

    Qt::ShortcutContext context() const;
    void setContext(Qt::ShortcutContext context);

Q_SIGNALS:
    void sequenceChanged();
    Q_REVISION(2, 9) void sequencesChanged();
    Q_REVISION(6, 10) void nativeTextChanged();
    Q_REVISION(6, 10) void portableTextChanged();
    void enabledChanged();
    void autoRepeatChanged();
    void contextChanged();

    void activated();
    void activatedAmbiguously();

protected:
    void classBegin() override;
    void componentComplete() override;
    bool event(QEvent *event) override;

    struct Shortcut {
        Shortcut() : id(0) { }
        bool matches(QShortcutEvent *event) const;
        int id;
        QVariant userValue;
        QKeySequence keySequence;
    };

    void setEnabled(Shortcut &shortcut, bool enabled);
    void setAutoRepeat(Shortcut &shortcut, bool repeat);

    void grabShortcut(Shortcut &shortcut, Qt::ShortcutContext context);
    void ungrabShortcut(Shortcut &shortcut);

private:
    bool m_enabled;
    bool m_completed;
    bool m_autorepeat;
    Qt::ShortcutContext m_context;
    Shortcut m_shortcut;
    QVector<Shortcut> m_shortcuts;
};

QT_END_NAMESPACE

#endif // QQUICKSHORTCUT_P_H

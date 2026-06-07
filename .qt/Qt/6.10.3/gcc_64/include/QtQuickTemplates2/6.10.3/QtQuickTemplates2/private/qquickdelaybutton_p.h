// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDELAYBUTTON_P_H
#define QQUICKDELAYBUTTON_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickTransition;
class QQuickDelayButtonPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickDelayButton : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged FINAL)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged FINAL)
    Q_PROPERTY(QQuickTransition *transition READ transition WRITE setTransition NOTIFY transitionChanged FINAL)
    QML_NAMED_ELEMENT(DelayButton)
    QML_ADDED_IN_VERSION(2, 2)

public:
    explicit QQuickDelayButton(QQuickItem *parent = nullptr);

    int delay() const;
    void setDelay(int delay);

    qreal progress() const;
    void setProgress(qreal progress);

    QQuickTransition *transition() const;
    void setTransition(QQuickTransition *transition);

Q_SIGNALS:
    void activated();
    void delayChanged();
    void progressChanged();
    void transitionChanged();

protected:
    void buttonChange(ButtonChange change) override;
    void nextCheckState() override;

    QFont defaultFont() const override;

private:
    Q_DISABLE_COPY(QQuickDelayButton)
    Q_DECLARE_PRIVATE(QQuickDelayButton)
};

QT_END_NAMESPACE

#endif // QQUICKDELAYBUTTON_P_H

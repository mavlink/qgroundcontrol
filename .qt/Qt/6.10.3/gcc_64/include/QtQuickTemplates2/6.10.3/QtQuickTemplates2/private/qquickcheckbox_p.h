// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCHECKBOX_P_H
#define QQUICKCHECKBOX_P_H

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

class QQuickCheckBoxPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickCheckBox : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate NOTIFY tristateChanged FINAL)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged FINAL)
    // 2.4 (Qt 5.11)
    Q_PROPERTY(QJSValue nextCheckState READ getNextCheckState WRITE setNextCheckState NOTIFY
                       nextCheckStateChanged FINAL REVISION(2, 4))
    QML_NAMED_ELEMENT(CheckBox)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickCheckBox(QQuickItem *parent = nullptr);

    bool isTristate() const;
    void setTristate(bool tristate);

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);
    QJSValue getNextCheckState() const;
    void setNextCheckState(const QJSValue &callback);

Q_SIGNALS:
    void tristateChanged();
    void checkStateChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void nextCheckStateChanged();

protected:
    QFont defaultFont() const override;

    void buttonChange(ButtonChange change) override;
    void nextCheckState() override;

private:
    Q_DISABLE_COPY(QQuickCheckBox)
    Q_DECLARE_PRIVATE(QQuickCheckBox)
};

QT_END_NAMESPACE

#endif // QQUICKCHECKBOX_P_H

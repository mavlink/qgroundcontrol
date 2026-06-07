// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCHECKBOX_H
#define QCHECKBOX_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractbutton.h>

QT_REQUIRE_CONFIG(checkbox);

QT_BEGIN_NAMESPACE


class QCheckBoxPrivate;
class QStyleOptionButton;

class Q_WIDGETS_EXPORT QCheckBox : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate)

public:
    explicit QCheckBox(QWidget *parent = nullptr);
    explicit QCheckBox(const QString &text, QWidget *parent = nullptr);
    ~QCheckBox();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setTristate(bool y = true);
    bool isTristate() const;

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

Q_SIGNALS:
#if QT_DEPRECATED_SINCE(6, 9)
    QT_MOC_COMPAT QT_DEPRECATED_VERSION_X_6_9("Use checkStateChanged() instead")
    void stateChanged(int);
#endif
    void checkStateChanged(Qt::CheckState);

protected:
    bool event(QEvent *e) override;
    bool hitButton(const QPoint &pos) const override;
    void checkStateSet() override;
    void nextCheckState() override;
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    virtual void initStyleOption(QStyleOptionButton *option) const;


private:
    Q_DECLARE_PRIVATE(QCheckBox)
    Q_DISABLE_COPY(QCheckBox)
    friend class QAccessibleButton;
};

QT_END_NAMESPACE

#endif // QCHECKBOX_H

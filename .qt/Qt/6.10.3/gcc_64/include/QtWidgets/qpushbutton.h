// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPUSHBUTTON_H
#define QPUSHBUTTON_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qabstractbutton.h>

QT_REQUIRE_CONFIG(pushbutton);

QT_BEGIN_NAMESPACE


class QPushButtonPrivate;
class QMenu;
class QStyleOptionButton;

class Q_WIDGETS_EXPORT QPushButton : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(bool autoDefault READ autoDefault WRITE setAutoDefault)
    Q_PROPERTY(bool default READ isDefault WRITE setDefault)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat)

public:
    explicit QPushButton(QWidget *parent = nullptr);
    explicit QPushButton(const QString &text, QWidget *parent = nullptr);
    QPushButton(const QIcon& icon, const QString &text, QWidget *parent = nullptr);
    ~QPushButton();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    bool autoDefault() const;
    void setAutoDefault(bool);
    bool isDefault() const;
    void setDefault(bool);

#if QT_CONFIG(menu)
    void setMenu(QMenu* menu);
    QMenu* menu() const;
#endif

    void setFlat(bool);
    bool isFlat() const;

public Q_SLOTS:
#if QT_CONFIG(menu)
    void showMenu();
#endif

protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    virtual void initStyleOption(QStyleOptionButton *option) const;
    bool hitButton(const QPoint &pos) const override;
    QPushButton(QPushButtonPrivate &dd, QWidget* parent = nullptr);

private:
    Q_DISABLE_COPY(QPushButton)
    Q_DECLARE_PRIVATE(QPushButton)
};

QT_END_NAMESPACE

#endif // QPUSHBUTTON_H

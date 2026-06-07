// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

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
#include <QtCore/qcoreapplication.h>
#include <QtWidgets/qaccessiblewidget.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QAbstractButton;
class QLineEdit;
class QToolButton;
class QGroupBox;
class QMenu;
class QMessageBox;
class QProgressBar;

#if QT_CONFIG(abstractbutton)
class QAccessibleButton : public QAccessibleWidgetV2
{
    Q_DECLARE_TR_FUNCTIONS(QAccessibleButton)
public:
    QAccessibleButton(QWidget *w);

    QString text(QAccessible::Text t) const override;
    QAccessible::State state() const override;
    QRect rect() const override;
    QAccessible::Role role() const override;

    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;

protected:
    QAbstractButton *button() const;
};
#endif

#if QT_CONFIG(toolbutton)
class QAccessibleToolButton : public QAccessibleButton
{
public:
    QAccessibleToolButton(QWidget *w);

    QAccessible::State state() const override;
    QAccessible::Role role() const override;

    int childCount() const override;
    QAccessibleInterface *child(int index) const override;

    // QAccessibleActionInterface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;

protected:
    QToolButton *toolButton() const;

    bool isSplitButton() const;
#if QT_CONFIG(menu)
    QMenu *menu() const;
#endif
};
#endif // QT_CONFIG(toolbutton)

class QAccessibleDisplay : public QAccessibleWidgetV2, public QAccessibleImageInterface
{
public:
    explicit QAccessibleDisplay(QWidget *w, QAccessible::Role role = QAccessible::StaticText);

    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;

    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
    relations(QAccessible::Relation match = QAccessible::AllRelations) const override;
    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleImageInterface
    QString imageDescription() const override;
    QSize imageSize() const override;
    QPoint imagePosition() const override;
};

#if QT_CONFIG(groupbox)
class QAccessibleGroupBox : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleGroupBox(QWidget *w);

    QAccessible::State state() const override;
    QAccessible::Role role() const override;
    QString text(QAccessible::Text t) const override;

    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
    relations(QAccessible::Relation match = QAccessible::AllRelations) const override;

    //QAccessibleActionInterface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &) const override;

private:
    QGroupBox *groupBox() const;
};
#endif

#if QT_CONFIG(lineedit)
class QAccessibleLineEdit : public QAccessibleWidgetV2, public QAccessibleTextInterface, public QAccessibleEditableTextInterface
{
public:
    explicit QAccessibleLineEdit(QWidget *o, const QString &name = QString());

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;
    QAccessible::State state() const override;
    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleTextInterface
    void addSelection(int startOffset, int endOffset) override;
    QString attributes(int offset, int *startOffset, int *endOffset) const override;
    int cursorPosition() const override;
    QRect characterRect(int offset) const override;
    int selectionCount() const override;
    int offsetAtPoint(const QPoint &point) const override;
    void selection(int selectionIndex, int *startOffset, int *endOffset) const override;
    QString text(int startOffset, int endOffset) const override;
    QString textBeforeOffset (int offset, QAccessible::TextBoundaryType boundaryType,
            int *startOffset, int *endOffset) const override;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
            int *startOffset, int *endOffset) const override;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
            int *startOffset, int *endOffset) const override;
    void removeSelection(int selectionIndex) override;
    void setCursorPosition(int position) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;
    int characterCount() const override;
    void scrollToSubstring(int startIndex, int endIndex) override;

    // QAccessibleEditableTextInterface
    void deleteText(int startOffset, int endOffset) override;
    void insertText(int offset, const QString &text) override;
    void replaceText(int startOffset, int endOffset, const QString &text) override;
protected:
    QLineEdit *lineEdit() const;
    friend class QAccessibleAbstractSpinBox;
};
#endif // QT_CONFIG(lineedit)

#if QT_CONFIG(progressbar)
class QAccessibleProgressBar : public QAccessibleDisplay, public QAccessibleValueInterface
{
public:
    explicit QAccessibleProgressBar(QWidget *o);
    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleValueInterface
    QVariant currentValue() const override;
    QVariant maximumValue() const override;
    QVariant minimumValue() const override;
    QVariant minimumStepSize() const override;
    void setCurrentValue(const QVariant &) override {}

protected:
    QProgressBar *progressBar() const;
};
#endif

class QWindowContainer;
class QAccessibleWindowContainer : public QAccessibleWidgetV2
{
public:
    QAccessibleWindowContainer(QWidget *w);
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    QAccessibleInterface *child(int i) const override;

private:
    QWindowContainer *container() const;
};

#if QT_CONFIG(messagebox)
class QAccessibleMessageBox : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleMessageBox(QWidget *widget);

    QString text(QAccessible::Text t) const override;

    QMessageBox *messageBox() const;
};
#endif

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE

#endif // SIMPLEWIDGETS_H

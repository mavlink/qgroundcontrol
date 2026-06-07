// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEWIDGETS_H
#define QACCESSIBLEWIDGETS_H

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
#include <QtWidgets/qaccessiblewidget.h>

#if QT_CONFIG(accessibility)

#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

class QTextEdit;
class QStackedWidget;
class QToolBox;
class QMdiArea;
class QMdiSubWindow;
class QRubberBand;
class QTextBrowser;
class QCalendarWidget;
class QAbstractItemView;
class QDockWidget;
class QDockWidgetLayout;
class QMainWindow;
class QPlainTextEdit;
class QTextCursor;
class QTextDocument;

#ifndef QT_NO_CURSOR
class QAccessibleTextWidget : public QAccessibleWidgetV2,
                              public QAccessibleTextInterface,
                              public QAccessibleEditableTextInterface
{
public:
    QAccessibleTextWidget(QWidget *o, QAccessible::Role r = QAccessible::EditableText, const QString &name = QString());

    QAccessible::State state() const override;

    // QAccessibleTextInterface
    //  selection
    void selection(int selectionIndex, int *startOffset, int *endOffset) const override;
    int selectionCount() const override;
    void addSelection(int startOffset, int endOffset) override;
    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;

    // cursor
    int cursorPosition() const override;
    void setCursorPosition(int position) override;

    // text
    QString text(int startOffset, int endOffset) const override;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                             int *startOffset, int *endOffset) const override;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                            int *startOffset, int *endOffset) const override;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                         int *startOffset, int *endOffset) const override;
    int characterCount() const override;

    // character <-> geometry
    QRect characterRect(int offset) const override;
    int offsetAtPoint(const QPoint &point) const override;

    QString attributes(int offset, int *startOffset, int *endOffset) const override;

    // QAccessibleEditableTextInterface
    void deleteText(int startOffset, int endOffset) override;
    void insertText(int offset, const QString &text) override;
    void replaceText(int startOffset, int endOffset, const QString &text) override;

    using QAccessibleWidgetV2::text;

protected:
    QTextCursor textCursorForRange(int startOffset, int endOffset) const;
    virtual QPoint scrollBarPosition() const;
    // return the current text cursor at the caret position including a potential selection
    virtual QTextCursor textCursor() const = 0;
    virtual void setTextCursor(const QTextCursor &) = 0;
    virtual QTextDocument *textDocument() const = 0;
    virtual QWidget *viewport() const = 0;
};

#if QT_CONFIG(textedit)
class QAccessiblePlainTextEdit : public QAccessibleTextWidget
{
public:
    explicit QAccessiblePlainTextEdit(QWidget *o);

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;
    QAccessible::State state() const override;

    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleTextInterface
    void scrollToSubstring(int startIndex, int endIndex) override;

    using QAccessibleTextWidget::text;

protected:
    QPlainTextEdit *plainTextEdit() const;

    QPoint scrollBarPosition() const override;
    QTextCursor textCursor() const override;
    void setTextCursor(const QTextCursor &textCursor) override;
    QTextDocument *textDocument() const override;
    QWidget *viewport() const override;
};

class QAccessibleTextEdit : public QAccessibleTextWidget
{
public:
    explicit QAccessibleTextEdit(QWidget *o);

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;
    QAccessible::State state() const override;

    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleTextInterface
    void scrollToSubstring(int startIndex, int endIndex) override;

    using QAccessibleTextWidget::text;

protected:
    QTextEdit *textEdit() const;

    QPoint scrollBarPosition() const override;
    QTextCursor textCursor() const override;
    void setTextCursor(const QTextCursor &textCursor) override;
    QTextDocument *textDocument() const override;
    QWidget *viewport() const override;
};
#endif // QT_CONFIG(textedit)
#endif  //QT_NO_CURSOR

class QAccessibleStackedWidget : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleStackedWidget(QWidget *widget);

    QAccessibleInterface *childAt(int x, int y) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    QAccessibleInterface *child(int index) const override;

protected:
    QStackedWidget *stackedWidget() const;
};

class QAccessibleToolBox : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleToolBox(QWidget *widget);

// FIXME we currently expose the toolbox but it is not keyboard navigatable
// and the accessible hierarchy is not exactly beautiful.
//    int childCount() const;
//    QAccessibleInterface *child(int index) const;
//    int indexOfChild(const QAccessibleInterface *child) const;

protected:
    QToolBox *toolBox() const;
};

#if QT_CONFIG(mdiarea)
class QAccessibleMdiArea : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleMdiArea(QWidget *widget);

    int childCount() const override;
    QAccessibleInterface *child(int index) const override;
    int indexOfChild(const QAccessibleInterface *child) const override;

protected:
    QMdiArea *mdiArea() const;
};

class QAccessibleMdiSubWindow : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleMdiSubWindow(QWidget *widget);

    QString text(QAccessible::Text textType) const override;
    void setText(QAccessible::Text textType, const QString &text) override;
    QAccessible::State state() const override;
    int childCount() const override;
    QAccessibleInterface *child(int index) const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    QRect rect() const override;

protected:
    QMdiSubWindow *mdiSubWindow() const;
};
#endif // QT_CONFIG(mdiarea)

#if QT_CONFIG(dialogbuttonbox)
class QAccessibleDialogButtonBox : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleDialogButtonBox(QWidget *widget);
};
#endif

#if QT_CONFIG(textbrowser) && !defined(QT_NO_CURSOR)
class QAccessibleTextBrowser : public QAccessibleTextEdit
{
public:
    explicit QAccessibleTextBrowser(QWidget *widget);

    QAccessible::Role role() const override;
};
#endif // QT_CONFIG(textbrowser) && QT_NO_CURSOR

#if QT_CONFIG(calendarwidget)
class QAccessibleCalendarWidget : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleCalendarWidget(QWidget *widget);

    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;

    QAccessibleInterface *child(int index) const override;

protected:
    QCalendarWidget *calendarWidget() const;

private:
    QAbstractItemView *calendarView() const;
    QWidget *navigationBar() const;
};
#endif // QT_CONFIG(calendarwidget)

#if QT_CONFIG(dockwidget)
class QAccessibleDockWidget: public QAccessibleWidgetV2
{
public:
    explicit QAccessibleDockWidget(QWidget *widget);
    QAccessibleInterface *child(int index) const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    int childCount() const override;
    QRect rect () const override;
    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override;

    QDockWidget *dockWidget() const;
protected:
    QDockWidgetLayout *dockWidgetLayout() const;
};

#endif // QT_CONFIG(dockwidget)

#if QT_CONFIG(mainwindow)
class QAccessibleMainWindow : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleMainWindow(QWidget *widget);

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    QMainWindow *mainWindow() const;

};
#endif // QT_CONFIG(mainwindow)

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE

#endif // QACESSIBLEWIDGETS_H

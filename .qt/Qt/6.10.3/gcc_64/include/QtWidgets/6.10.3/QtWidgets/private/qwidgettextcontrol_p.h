// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGETTEXTCONTROL_P_H
#define QWIDGETTEXTCONTROL_P_H

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
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextoption.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qtextformat.h>
#if QT_CONFIG(textedit)
#include <QtWidgets/qtextedit.h>
#endif
#if QT_CONFIG(menu)
#include <QtWidgets/qmenu.h>
#endif
#include <QtCore/qrect.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextdocumentfragment.h>
#include <QtGui/qclipboard.h>
#include <QtCore/qmimedata.h>
#include <QtGui/private/qinputcontrol_p.h>

QT_REQUIRE_CONFIG(widgettextcontrol);

QT_BEGIN_NAMESPACE


class QStyleSheet;
class QTextDocument;
class QMenu;
class QWidgetTextControlPrivate;
class QAbstractScrollArea;
class QEvent;
class QRegularExpression;
class QTimerEvent;

class Q_WIDGETS_EXPORT QWidgetTextControl : public QInputControl
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWidgetTextControl)
#ifndef QT_NO_TEXTHTMLPARSER
    Q_PROPERTY(QString html READ toHtml WRITE setHtml NOTIFY textChanged USER true)
#endif
    Q_PROPERTY(bool overwriteMode READ overwriteMode WRITE setOverwriteMode)
    Q_PROPERTY(bool acceptRichText READ acceptRichText WRITE setAcceptRichText)
    Q_PROPERTY(int cursorWidth READ cursorWidth WRITE setCursorWidth)
    Q_PROPERTY(Qt::TextInteractionFlags textInteractionFlags READ textInteractionFlags
               WRITE setTextInteractionFlags)
    Q_PROPERTY(bool openExternalLinks READ openExternalLinks WRITE setOpenExternalLinks)
    Q_PROPERTY(bool ignoreUnusedNavigationEvents READ ignoreUnusedNavigationEvents
               WRITE setIgnoreUnusedNavigationEvents)
public:
    explicit QWidgetTextControl(QObject *parent = nullptr);
    explicit QWidgetTextControl(const QString &text, QObject *parent = nullptr);
    explicit QWidgetTextControl(QTextDocument *doc, QObject *parent = nullptr);
    virtual ~QWidgetTextControl();

    void setDocument(QTextDocument *document);
    QTextDocument *document() const;

    void setTextCursor(const QTextCursor &cursor, bool selectionClipboard = false);
    QTextCursor textCursor() const;

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    void mergeCurrentCharFormat(const QTextCharFormat &modifier);

    void setCurrentCharFormat(const QTextCharFormat &format);
    QTextCharFormat currentCharFormat() const;

    bool find(const QString &exp, QTextDocument::FindFlags options = { });
#if QT_CONFIG(regularexpression)
    bool find(const QRegularExpression &exp, QTextDocument::FindFlags options = { });
#endif

    QString toPlainText() const;
#ifndef QT_NO_TEXTHTMLPARSER
    QString toHtml() const;
#endif
#if QT_CONFIG(textmarkdownwriter)
    QString toMarkdown(QTextDocument::MarkdownFeatures features = QTextDocument::MarkdownDialectGitHub) const;
#endif

    virtual void ensureCursorVisible();

    Q_INVOKABLE virtual QVariant loadResource(int type, const QUrl &name);
#ifndef QT_NO_CONTEXTMENU
    QMenu *createStandardContextMenu(const QPointF &pos, QWidget *parent);
#endif

    QTextCursor cursorForPosition(const QPointF &pos) const;
    QRectF cursorRect(const QTextCursor &cursor) const;
    QRectF cursorRect() const;
    QRectF selectionRect(const QTextCursor &cursor) const;
    QRectF selectionRect() const;

    virtual QString anchorAt(const QPointF &pos) const;
    QPointF anchorPosition(const QString &name) const;

    QString anchorAtCursor() const;

    QTextBlock blockWithMarkerAt(const QPointF &pos) const;

    bool overwriteMode() const;
    void setOverwriteMode(bool overwrite);

    int cursorWidth() const;
    void setCursorWidth(int width);

    bool acceptRichText() const;
    void setAcceptRichText(bool accept);

#if QT_CONFIG(textedit)
    void setExtraSelections(const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections() const;
#endif

    void setTextWidth(qreal width);
    qreal textWidth() const;
    QSizeF size() const;

    void setOpenExternalLinks(bool open);
    bool openExternalLinks() const;

    void setIgnoreUnusedNavigationEvents(bool ignore);
    bool ignoreUnusedNavigationEvents() const;

    void moveCursor(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    bool canPaste() const;

    void setCursorIsFocusIndicator(bool b);
    bool cursorIsFocusIndicator() const;

    void setDragEnabled(bool enabled);
    bool isDragEnabled() const;

    bool isWordSelectionEnabled() const;
    void setWordSelectionEnabled(bool enabled);

    bool isPreediting();

    void print(QPagedPaintDevice *printer) const;

    virtual int hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const;
    virtual QRectF blockBoundingRect(const QTextBlock &block) const;
    QAbstractTextDocumentLayout::PaintContext getPaintContext(QWidget *widget) const;

public Q_SLOTS:
    void setPlainText(const QString &text);
#if QT_CONFIG(textmarkdownreader)
    void setMarkdown(const QString &text);
#endif
    void setHtml(const QString &text);

#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy();
    void paste(QClipboard::Mode mode = QClipboard::Clipboard);
#endif

    void undo();
    void redo();

    void clear();
    void selectAll();

    void insertPlainText(const QString &text);
#ifndef QT_NO_TEXTHTMLPARSER
    void insertHtml(const QString &text);
#endif

    void append(const QString &text);
    void appendHtml(const QString &html);
    void appendPlainText(const QString &text);

    void adjustSize();

Q_SIGNALS:
    void textChanged();
    void undoAvailable(bool b);
    void redoAvailable(bool b);
    void currentCharFormatChanged(const QTextCharFormat &format);
    void copyAvailable(bool b);
    void selectionChanged();
    void cursorPositionChanged();

    // control signals
    void updateRequest(const QRectF &rect = QRectF());
    void documentSizeChanged(const QSizeF &);
    void blockCountChanged(int newBlockCount);
    void visibilityRequest(const QRectF &rect);
    void microFocusChanged();
    void linkActivated(const QString &link);
    void linkHovered(const QString &);
    void blockMarkerHovered(const QTextBlock &block);
    void modificationChanged(bool m);

public:
    // control properties
    QPalette palette() const;
    void setPalette(const QPalette &pal);

    virtual void processEvent(QEvent *e, const QTransform &transform, QWidget *contextWidget = nullptr);
    void processEvent(QEvent *e, const QPointF &coordinateOffset = QPointF(), QWidget *contextWidget = nullptr);

    // control methods
    void drawContents(QPainter *painter, const QRectF &rect = QRectF(), QWidget *widget = nullptr);

    void setFocus(bool focus, Qt::FocusReason = Qt::OtherFocusReason);

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const;

    virtual QMimeData *createMimeDataFromSelection() const;
    virtual bool canInsertFromMimeData(const QMimeData *source) const;
    virtual void insertFromMimeData(const QMimeData *source);

    bool setFocusToAnchor(const QTextCursor &newCursor);
    bool setFocusToNextOrPreviousAnchor(bool next);
    bool findNextPrevAnchor(const QTextCursor& from, bool next, QTextCursor& newAnchor);

protected:
    virtual void timerEvent(QTimerEvent *e) override;

    virtual bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY_MOVE(QWidgetTextControl)
    Q_PRIVATE_SLOT(d_func(), void _q_deleteSelected())
    Q_PRIVATE_SLOT(d_func(), void _q_copyLink())
};


#ifndef QT_NO_CONTEXTMENU
class QUnicodeControlCharacterMenu : public QMenu
{
    Q_OBJECT
public:
    QUnicodeControlCharacterMenu(QObject *editWidget, QWidget *parent);

private Q_SLOTS:
    void menuActionTriggered();

private:
    QObject *editWidget;
};
#endif // QT_NO_CONTEXTMENU


// also used by QLabel
class QTextEditMimeData : public QMimeData
{
public:
    inline QTextEditMimeData(const QTextDocumentFragment &aFragment) : fragment(aFragment) {}

    virtual QStringList formats() const override;
    bool hasFormat(const QString &format) const override;

protected:
    virtual QVariant retrieveData(const QString &mimeType, QMetaType type) const override;
private:
    void setup() const;

    mutable QTextDocumentFragment fragment;
};

#ifndef QT_NO_CONTEXTMENU
// also used by QLineEdit
void setActionIcon(QAction *action, const QString &name);
#endif // QT_NO_CONTEXTMENU

QT_END_NAMESPACE

#endif // QWidgetTextControl_H

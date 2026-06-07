// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTCONTROL_P_H
#define QQUICKTEXTCONTROL_P_H

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

#include <QtGui/qtextdocument.h>
#include <QtGui/qtextoption.h>
#include <QtGui/qtextcursor.h>
#include <QtGui/qtextformat.h>
#include <QtCore/qrect.h>
#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextdocumentfragment.h>
#include <QtGui/qclipboard.h>
#include <QtGui/private/qinputcontrol_p.h>
#include <QtCore/qmimedata.h>

QT_BEGIN_NAMESPACE


class QStyleSheet;
class QTextDocument;
class QQuickTextControlPrivate;
class QAbstractScrollArea;
class QEvent;
class QTimerEvent;
class QTransform;

class Q_AUTOTEST_EXPORT QQuickTextControl : public QInputControl
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickTextControl)
public:
    explicit QQuickTextControl(QTextDocument *doc, QObject *parent = nullptr);
    virtual ~QQuickTextControl();

    QTextDocument *document() const;
    void setDocument(QTextDocument *doc);

    void setTextCursor(const QTextCursor &cursor);
    QTextCursor textCursor() const;

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    QString toPlainText() const;

#if QT_CONFIG(texthtmlparser)
    QString toHtml() const;
#endif
#if QT_CONFIG(textmarkdownwriter)
    QString toMarkdown() const;
#endif

    bool hasImState() const;
    bool overwriteMode() const;
    void setOverwriteMode(bool overwrite);
    bool cursorVisible() const;
    void setCursorVisible(bool visible);
    QRectF anchorRect() const;
    QRectF cursorRect(const QTextCursor &cursor) const;
    QRectF cursorRect() const;
    QRectF selectionRect(const QTextCursor &cursor) const;
    QRectF selectionRect() const;

    QString hoveredLink() const;
    QString anchorAt(const QPointF &pos) const;
    QTextBlock blockWithMarkerAt(const QPointF &pos) const;

    void setCursorWidth(int width);

    void setAcceptRichText(bool accept);

    void moveCursor(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    bool canPaste() const;

    void setCursorIsFocusIndicator(bool b);
    void setWordSelectionEnabled(bool enabled);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void setTouchDragSelectionEnabled(bool enabled);
#endif

    void updateCursorRectangle(bool force);

    virtual int hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const;
    virtual QRectF blockBoundingRect(const QTextBlock &block) const;

    QString preeditText() const;

public Q_SLOTS:
    void setPlainText(const QString &text);
    void setMarkdownText(const QString &text);
    void setHtml(const QString &text);

#if QT_CONFIG(clipboard)
    void cut();
    void copy();
    void paste(QClipboard::Mode mode = QClipboard::Clipboard);
#endif

    void undo();
    void redo();
    void clear();

    void selectAll();

Q_SIGNALS:
    void textChanged();
    void preeditTextChanged();
    void contentsChange(int from, int charsRemoved, int charsAdded);
    void undoAvailable(bool b);
    void redoAvailable(bool b);
    void currentCharFormatChanged(const QTextCharFormat &format);
    void copyAvailable(bool b);
    void selectionChanged();
    void cursorPositionChanged();
    void overwriteModeChanged(bool overwriteMode);

    // control signals
    void updateCursorRequest();
    void updateRequest();
    void cursorRectangleChanged();
    void linkActivated(const QString &link);
    void linkHovered(const QString &link);
    void markerClicked();
    void markerHovered(bool marker);

public:
    virtual void processEvent(QEvent *e, const QTransform &transform);
    void processEvent(QEvent *e, const QPointF &coordinateOffset = QPointF());

#if QT_CONFIG(im)
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;
    Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery query, const QVariant &argument) const;
#endif

    virtual QMimeData *createMimeDataFromSelection() const;
    virtual bool canInsertFromMimeData(const QMimeData *source) const;
    virtual void insertFromMimeData(const QMimeData *source);

    bool cursorOn() const;

    bool isBeingEdited();

protected:
    void timerEvent(QTimerEvent *e) override;

    bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY(QQuickTextControl)
    Q_PRIVATE_SLOT(d_func(), void _q_updateCurrentCharFormatAndSelection())
    Q_PRIVATE_SLOT(d_func(), void _q_updateCursorPosChanged(const QTextCursor &))
};


// also used by QLabel
class QQuickTextEditMimeData : public QMimeData
{
public:
    inline QQuickTextEditMimeData(const QTextDocumentFragment &aFragment) : fragment(aFragment) {}

    QStringList formats() const override;

protected:
    QVariant retrieveData(const QString &mimeType, QMetaType type) const override;

private:
    void setup() const;

    mutable QTextDocumentFragment fragment;
};

QT_END_NAMESPACE

#endif // QQuickTextControl_H

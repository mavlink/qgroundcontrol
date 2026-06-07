// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTEDIT_P_P_H
#define QQUICKTEXTEDIT_P_P_H

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

#include "qquicktextedit_p.h"
#include "qquickimplicitsizeitem_p_p.h"
#include "qquicktextutil_p.h"

#include <QtQuick/private/qquicktextselection_p.h>

#include <QtQml/qqml.h>
#include <QtCore/qlist.h>
#include <private/qlazilyallocated_p.h>
#include <private/qquicktextdocument_p.h>

#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible.h>
#endif

#include <limits>

QT_BEGIN_NAMESPACE
class QTextLayout;
class QQuickPixmap;
class QQuickTextControl;
class QSGInternalTextNode;
class QQuickTextNodeEngine;

class Q_QUICK_EXPORT QQuickTextEditPrivate : public QQuickImplicitSizeItemPrivate
#if QT_CONFIG(accessibility)
    , public QAccessible::ActivationObserver
#endif
{
public:
    Q_DECLARE_PUBLIC(QQuickTextEdit)

    typedef QQuickTextEdit Public;

    struct Node {
        explicit Node(int startPos = std::numeric_limits<int>::max(),
                      QSGInternalTextNode *node = nullptr)
            : m_node(node), m_startPos(startPos) { }
        QSGInternalTextNode *textNode() const { return m_node; }
        void moveStartPos(int delta) { Q_ASSERT(m_startPos + delta > 0); m_startPos += delta; }
        int startPos() const { return m_startPos; }
        void setDirty() { m_dirty = true; }
        bool dirty() const { return m_dirty; }

    private:
        QSGInternalTextNode *m_node;
        int m_startPos;
        bool m_dirty = false;

#ifndef QT_NO_DEBUG_STREAM
        friend QDebug Q_QUICK_EXPORT operator<<(QDebug, const Node &);
#endif
    };
    typedef QList<Node>::iterator TextNodeIterator;

    struct ExtraData {
        ExtraData();

        qreal padding = 0;
        qreal topPadding = 0;
        qreal leftPadding = 0;
        qreal rightPadding = 0;
        qreal bottomPadding = 0;
        bool explicitTopPadding : 1;
        bool explicitLeftPadding : 1;
        bool explicitRightPadding : 1;
        bool explicitBottomPadding : 1;
        bool implicitResize : 1;
    };
    QLazilyAllocated<ExtraData> extra;


    QQuickTextEditPrivate()
        : dirty(false), richText(false), cursorVisible(false), cursorPending(false)
        , focusOnPress(true), persistentSelection(false), requireImplicitWidth(false)
        , selectByMouse(true), canPaste(false), canPasteValid(false), hAlignImplicit(true)
        , textCached(true), inLayout(false), selectByKeyboard(false), selectByKeyboardSet(false)
        , hadSelection(false), markdownText(false), inResize(false), ownsDocument(false)
        , containsUnscalableGlyphs(false)
    {
#if QT_CONFIG(accessibility)
        QAccessible::installActivationObserver(this);
#endif
    }

    ~QQuickTextEditPrivate()
    {
#if QT_CONFIG(accessibility)
        QAccessible::removeActivationObserver(this);
#endif
    }

    static QQuickTextEditPrivate *get(QQuickTextEdit *item) {
        return static_cast<QQuickTextEditPrivate *>(QObjectPrivate::get(item)); }

    void init();

    void resetInputMethod();
    void updateDefaultTextOption();
    void onDocumentStatusChanged();
    void relayoutDocument();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickTextEdit::HAlignment, bool forceAlign = false);
    void mirrorChange() override;
    bool transformChanged(QQuickItem *transformedItem) override;
    qreal getImplicitWidth() const override;
    Qt::LayoutDirection textDirection(const QString &text) const;
    bool isLinkHoveredConnected();

#if QT_CONFIG(cursor)
    void updateMouseCursorShape();
#endif

    void setNativeCursorEnabled(bool) {}
    void handleFocusEvent(QFocusEvent *event);
    void addCurrentTextNodeToRoot(QQuickTextNodeEngine *, QSGTransformNode *, QSGInternalTextNode *, TextNodeIterator&, int startPos);
    QSGInternalTextNode* createTextNode();

#if QT_CONFIG(im)
    Qt::InputMethodHints effectiveInputMethodHints() const;
#endif

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
#endif

    inline qreal padding() const { return extra.isAllocated() ? extra->padding : 0.0; }
    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);

    bool isImplicitResizeEnabled() const;
    void setImplicitResizeEnabled(bool enabled);

    QColor color = QRgb(0xFF000000);
    QColor selectionColor = QRgb(0xFF000080);
    QColor selectedTextColor = QRgb(0xFFFFFFFF);

    QSizeF contentSize;

    qreal textMargin = 0;
    qreal xoff = 0;
    qreal yoff = 0;

    QString text;
    QUrl baseUrl;
    QFont sourceFont;
    QFont font;

    QQmlComponent* cursorComponent = nullptr;
    QQuickItem* cursorItem = nullptr;
    QTextDocument *document = nullptr;
    QQuickTextControl *control = nullptr;
    QQuickTextDocument *quickDocument = nullptr;
    mutable QQuickTextSelection *cursorSelection = nullptr;
    QList<Node> textNodeMap;
    QList<QQuickPixmap *> pixmapsInProgress;

    int lastSelectionStart = 0;
    int lastSelectionEnd = 0;
    int lineCount = 0;
    int firstBlockInViewport = -1;   // can be wrong after scrolling sometimes
    int firstBlockPastViewport = -1; // only for the autotest
    int renderedBlockCount = -1;     // only for the autotest
    QRectF renderedRegion;

    enum UpdateType {
        UpdateNone,
        UpdateOnlyPreprocess,
        UpdatePaintNode,
        UpdateAll
    };

    QQuickTextEdit::HAlignment hAlign = QQuickTextEdit::AlignLeft;
    QQuickTextEdit::VAlignment vAlign = QQuickTextEdit::AlignTop;
    QQuickTextEdit::TextFormat format = QQuickTextEdit::PlainText;
    QQuickTextEdit::WrapMode wrapMode = QQuickTextEdit::NoWrap;
    QQuickTextEdit::RenderType renderType = QQuickTextUtil::textRenderType<QQuickTextEdit>();
    Qt::LayoutDirection contentDirection = Qt::LayoutDirectionAuto;
    QQuickTextEdit::SelectionMode mouseSelectionMode = QQuickTextEdit::SelectCharacters;
#if QT_CONFIG(im)
    Qt::InputMethodHints inputMethodHints = Qt::ImhNone;
#endif
    UpdateType updateType = UpdatePaintNode;

    bool dirty : 1;
    bool richText : 1;
    bool cursorVisible : 1;
    bool cursorPending : 1;
    bool focusOnPress : 1;
    bool persistentSelection : 1;
    bool requireImplicitWidth:1;
    bool selectByMouse:1;
    bool canPaste:1;
    bool canPasteValid:1;
    bool hAlignImplicit:1;
    bool textCached:1;
    bool inLayout:1;
    bool selectByKeyboard:1;
    bool selectByKeyboardSet:1;
    bool hadSelection : 1;
    bool markdownText : 1;
    bool inResize : 1;
    bool ownsDocument : 1;
    bool containsUnscalableGlyphs : 1;

    static const int largeTextSizeThreshold;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_QUICK_EXPORT operator<<(QDebug debug, const QQuickTextEditPrivate::Node &);
#endif

QT_END_NAMESPACE

#endif // QQUICKTEXTEDIT_P_P_H

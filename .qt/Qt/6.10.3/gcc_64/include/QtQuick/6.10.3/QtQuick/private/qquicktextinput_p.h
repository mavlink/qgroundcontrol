// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTINPUT_P_H
#define QQUICKTEXTINPUT_P_H

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

#include "qquickimplicitsizeitem_p.h"
#include "qquicktextinterface_p.h"
#include <QtGui/qtextoption.h>
#if QT_CONFIG(validator)
#include <QtGui/qvalidator.h>
#endif

QT_BEGIN_NAMESPACE

class QQuickTextInputPrivate;
class QQmlComponent;

class Q_QUICK_EXPORT QQuickTextInput : public QQuickImplicitSizeItem, public QQuickTextInterface
{
    Q_OBJECT
    Q_INTERFACES(QQuickTextInterface)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int length READ length NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ hAlign WRITE setHAlign RESET resetHAlign NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(HAlignment effectiveHorizontalAlignment READ effectiveHAlign NOTIFY effectiveHorizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ vAlign WRITE setVAlign NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool cursorVisible READ isCursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QQmlComponent *cursorDelegate READ cursorDelegate WRITE setCursorDelegate NOTIFY cursorDelegateChanged)
    Q_PROPERTY(bool overwriteMode READ overwriteMode WRITE setOverwriteMode NOTIFY overwriteModeChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectedTextChanged)

    Q_PROPERTY(int maximumLength READ maxLength WRITE setMaxLength NOTIFY maximumLengthChanged)
#if QT_CONFIG(validator)
    Q_PROPERTY(QValidator* validator READ validator WRITE setValidator NOTIFY validatorChanged)
#endif
    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask NOTIFY inputMaskChanged)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged)

    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput NOTIFY acceptableInputChanged)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode NOTIFY echoModeChanged)
    Q_PROPERTY(bool activeFocusOnPress READ focusOnPress WRITE setFocusOnPress NOTIFY activeFocusOnPressChanged)
    Q_PROPERTY(QString passwordCharacter READ passwordCharacter WRITE setPasswordCharacter NOTIFY passwordCharacterChanged)
    Q_PROPERTY(int passwordMaskDelay READ passwordMaskDelay WRITE setPasswordMaskDelay RESET resetPasswordMaskDelay NOTIFY passwordMaskDelayChanged REVISION(2, 4))
    Q_PROPERTY(QString displayText READ displayText NOTIFY displayTextChanged)
    Q_PROPERTY(QString preeditText READ preeditText NOTIFY preeditTextChanged REVISION(2, 7))
    Q_PROPERTY(bool autoScroll READ autoScroll WRITE setAutoScroll NOTIFY autoScrollChanged)
    Q_PROPERTY(bool selectByMouse READ selectByMouse WRITE setSelectByMouse NOTIFY selectByMouseChanged)
    Q_PROPERTY(SelectionMode mouseSelectionMode READ mouseSelectionMode WRITE setMouseSelectionMode NOTIFY mouseSelectionModeChanged)
    Q_PROPERTY(bool persistentSelection READ persistentSelection WRITE setPersistentSelection NOTIFY persistentSelectionChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY canPasteChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentSizeChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentSizeChanged)
    Q_PROPERTY(RenderType renderType READ renderType WRITE setRenderType NOTIFY renderTypeChanged)

    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged REVISION(2, 6))
    QML_NAMED_ELEMENT(TextInput)
    QML_ADDED_IN_VERSION(2, 0)
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QML_ADDED_IN_VERSION(6, 4)
#endif

public:
    QQuickTextInput(QQuickItem * parent=nullptr);
    ~QQuickTextInput();

    void componentComplete() override;

    enum EchoMode {//To match QLineEdit::EchoMode
        Normal,
        NoEcho,
        Password,
        PasswordEchoOnEdit
    };
    Q_ENUM(EchoMode)

    enum HAlignment {
        AlignLeft = Qt::AlignLeft,
        AlignRight = Qt::AlignRight,
        AlignHCenter = Qt::AlignHCenter
    };
    Q_ENUM(HAlignment)

    enum VAlignment {
        AlignTop = Qt::AlignTop,
        AlignBottom = Qt::AlignBottom,
        AlignVCenter = Qt::AlignVCenter
    };
    Q_ENUM(VAlignment)

    enum WrapMode {
        NoWrap = QTextOption::NoWrap,
        WordWrap = QTextOption::WordWrap,
        WrapAnywhere = QTextOption::WrapAnywhere,
        WrapAtWordBoundaryOrAnywhere = QTextOption::WrapAtWordBoundaryOrAnywhere, // COMPAT
        Wrap = QTextOption::WrapAtWordBoundaryOrAnywhere
    };
    Q_ENUM(WrapMode)

    enum SelectionMode {
        SelectCharacters,
        SelectWords
    };
    Q_ENUM(SelectionMode)

    enum CursorPosition {
        CursorBetweenCharacters,
        CursorOnCharacter
    };
    Q_ENUM(CursorPosition)

    enum RenderType { QtRendering,
                      NativeRendering,
                      CurveRendering
                    };
    Q_ENUM(RenderType)

    //Auxilliary functions needed to control the TextInput from QML
    Q_INVOKABLE void positionAt(QQmlV4FunctionPtr args) const;
    Q_INVOKABLE QRectF positionToRectangle(int pos) const;
    Q_INVOKABLE void moveCursorSelection(int pos);
    Q_INVOKABLE void moveCursorSelection(int pos, QQuickTextInput::SelectionMode mode);

    RenderType renderType() const;
    void setRenderType(RenderType renderType);

    QString text() const;
    void setText(const QString &);

    int length() const;

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &c);

    QColor selectionColor() const;
    void setSelectionColor(const QColor &c);

    QColor selectedTextColor() const;
    void setSelectedTextColor(const QColor &c);

    HAlignment hAlign() const;
    void setHAlign(HAlignment align);
    void resetHAlign();
    HAlignment effectiveHAlign() const;

    VAlignment vAlign() const;
    void setVAlign(VAlignment align);

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode w);

    bool isReadOnly() const;
    void setReadOnly(bool);

    bool isCursorVisible() const;
    void setCursorVisible(bool on);

    int cursorPosition() const;
    void setCursorPosition(int cp);

    QRectF cursorRectangle() const;

    int selectionStart() const;
    int selectionEnd() const;

    QString selectedText() const;

    int maxLength() const;
    void setMaxLength(int ml);

#if QT_CONFIG(validator)
    QValidator * validator() const;
    void setValidator(QValidator* v);
#endif

    QString inputMask() const;
    void setInputMask(const QString &im);

    EchoMode echoMode() const;
    void setEchoMode(EchoMode echo);

    QString passwordCharacter() const;
    void setPasswordCharacter(const QString &str);

    int passwordMaskDelay() const;
    void setPasswordMaskDelay(int delay);
    void resetPasswordMaskDelay();

    QString displayText() const;
    Q_REVISION(2, 7) QString preeditText() const;

    QQmlComponent* cursorDelegate() const;
    void setCursorDelegate(QQmlComponent*);

    bool overwriteMode() const;
    void setOverwriteMode(bool overwrite);

    bool focusOnPress() const;
    void setFocusOnPress(bool);

    bool autoScroll() const;
    void setAutoScroll(bool);

    bool selectByMouse() const;
    void setSelectByMouse(bool);

    SelectionMode mouseSelectionMode() const;
    void setMouseSelectionMode(SelectionMode mode);

    bool persistentSelection() const;
    void setPersistentSelection(bool persist);

    bool hasAcceptableInput() const;

#if QT_CONFIG(im)
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;
    Q_REVISION(2, 4) Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery query, const QVariant &argument) const;
#endif

    QRectF boundingRect() const override;
    QRectF clipRect() const override;

    bool canPaste() const;

    bool canUndo() const;
    bool canRedo() const;

    bool isInputMethodComposing() const;

    Qt::InputMethodHints inputMethodHints() const;
    void setInputMethodHints(Qt::InputMethodHints hints);

    Q_INVOKABLE QString getText(int start, int end) const;

    qreal contentWidth() const;
    qreal contentHeight() const;

    qreal padding() const;
    void setPadding(qreal padding);
    void resetPadding();

    qreal topPadding() const;
    void setTopPadding(qreal padding);
    void resetTopPadding();

    qreal leftPadding() const;
    void setLeftPadding(qreal padding);
    void resetLeftPadding();

    qreal rightPadding() const;
    void setRightPadding(qreal padding);
    void resetRightPadding();

    qreal bottomPadding() const;
    void setBottomPadding(qreal padding);
    void resetBottomPadding();

    void invalidate() override;

Q_SIGNALS:
    void textChanged();
    void cursorPositionChanged();
    void cursorRectangleChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void selectedTextChanged();
    void accepted();
    void acceptableInputChanged();
    Q_REVISION(2, 2) void editingFinished();
    Q_REVISION(2, 9) void textEdited();
    void colorChanged();
    void selectionColorChanged();
    void selectedTextColorChanged();
    void fontChanged(const QFont &font);
    void horizontalAlignmentChanged(QQuickTextInput::HAlignment alignment);
    void verticalAlignmentChanged(QQuickTextInput::VAlignment alignment);
    void wrapModeChanged();
    void readOnlyChanged(bool isReadOnly);
    void cursorVisibleChanged(bool isCursorVisible);
    void cursorDelegateChanged();
    void overwriteModeChanged(bool overwriteMode);
    void maximumLengthChanged(int maximumLength);
#if QT_CONFIG(validator)
    void validatorChanged();
#endif
    void inputMaskChanged(const QString &inputMask);
    void echoModeChanged(QQuickTextInput::EchoMode echoMode);
    void passwordCharacterChanged();
    Q_REVISION(2, 4) void passwordMaskDelayChanged(int delay);
    void displayTextChanged();
    Q_REVISION(2, 7) void preeditTextChanged();
    void activeFocusOnPressChanged(bool activeFocusOnPress);
    void autoScrollChanged(bool autoScroll);
    void selectByMouseChanged(bool selectByMouse);
    void mouseSelectionModeChanged(QQuickTextInput::SelectionMode mode);
    void persistentSelectionChanged();
    void canPasteChanged();
    void canUndoChanged();
    void canRedoChanged();
    void inputMethodComposingChanged();
    void effectiveHorizontalAlignmentChanged();
    void contentSizeChanged();
    void inputMethodHintsChanged();
    void renderTypeChanged();
    Q_REVISION(2, 6) void paddingChanged();
    Q_REVISION(2, 6) void topPaddingChanged();
    Q_REVISION(2, 6) void leftPaddingChanged();
    Q_REVISION(2, 6) void rightPaddingChanged();
    Q_REVISION(2, 6) void bottomPaddingChanged();

private:
    void invalidateFontCaches();
    void ensureActiveFocus(Qt::FocusReason reason);

protected:
    QQuickTextInput(QQuickTextInputPrivate &dd, QQuickItem *parent = nullptr);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void setOldSelectionDefault();
#endif

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent* ev) override;
#if QT_CONFIG(im)
    void inputMethodEvent(QInputMethodEvent *) override;
#endif
    void mouseUngrabEvent() override;
    bool event(QEvent *e) override;
    void focusOutEvent(QFocusEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void updatePolish() override;

public Q_SLOTS:
    void selectAll();
    void selectWord();
    void select(int start, int end);
    void deselect();
    bool isRightToLeft(int start, int end);
#if QT_CONFIG(clipboard)
    void cut();
    void copy();
    void paste();
#endif
    void undo();
    void redo();
    void insert(int position, const QString &text);
    void remove(int start, int end);
    Q_REVISION(2, 4) void ensureVisible(int position);
    Q_REVISION(2, 7) void clear();

private Q_SLOTS:
    void selectionChanged();
    void createCursor();
    void updateCursorRectangle(bool scroll = true);
    void q_canPasteChanged();
    void q_updateAlignment();
    void triggerPreprocess();

#if QT_CONFIG(validator)
    void q_validatorChanged();
#endif

private:
    friend class QQuickTextUtil;

    Q_DECLARE_PRIVATE(QQuickTextInput)
};

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
class QQuickPre64TextInput : public QQuickTextInput {
    Q_OBJECT
    QML_NAMED_ELEMENT(TextInput)
    QML_ADDED_IN_VERSION(2, 0)
    QML_REMOVED_IN_VERSION(6, 4)
public:
    QQuickPre64TextInput(QQuickItem *parent = nullptr);
};
#endif

QT_END_NAMESPACE

#endif // QQUICKTEXTINPUT_P_H

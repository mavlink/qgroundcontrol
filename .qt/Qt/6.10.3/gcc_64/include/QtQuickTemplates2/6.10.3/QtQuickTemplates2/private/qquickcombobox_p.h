// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOMBOBOX_P_H
#define QQUICKCOMBOBOX_P_H

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

#include <QtCore/qloggingcategory.h>
#include <QtQmlModels/private/qtqmlmodels-config_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcItemManagement)

class QValidator;
class QQuickPopup;
class QQmlInstanceModel;
class QQuickComboBoxPrivate;
class QQmlComponent;

class Q_QUICKTEMPLATES2_EXPORT QQuickComboBox : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QQmlInstanceModel *delegateModel READ delegateModel NOTIFY delegateModelChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged FINAL)
    Q_PROPERTY(QString displayText READ displayText WRITE setDisplayText RESET resetDisplayText NOTIFY displayTextChanged FINAL)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    Q_PROPERTY(QQuickPopup *popup READ popup WRITE setPopup NOTIFY popupChanged FINAL)
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat NOTIFY flatChanged FINAL REVISION(2, 1))
    // 2.2 (Qt 5.9)
    Q_PROPERTY(bool down READ isDown WRITE setDown RESET resetDown NOTIFY downChanged FINAL REVISION(2, 2))
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged FINAL REVISION(2, 2))
    Q_PROPERTY(QString editText READ editText WRITE setEditText RESET resetEditText NOTIFY editTextChanged FINAL REVISION(2, 2))
#if QT_CONFIG(validator)
    Q_PROPERTY(QValidator *validator READ validator WRITE setValidator NOTIFY validatorChanged FINAL REVISION(2, 2))
#endif
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged FINAL REVISION(2, 2))
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged FINAL REVISION(2, 2))
    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput NOTIFY acceptableInputChanged FINAL REVISION(2, 2))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitIndicatorWidth READ implicitIndicatorWidth NOTIFY implicitIndicatorWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitIndicatorHeight READ implicitIndicatorHeight NOTIFY implicitIndicatorHeightChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem,indicator,popup")
    // 2.14 (Qt 5.14)
    Q_PROPERTY(QVariant currentValue READ currentValue WRITE setCurrentValue NOTIFY currentValueChanged FINAL REVISION(2, 14))
    Q_PROPERTY(QString valueRole READ valueRole WRITE setValueRole NOTIFY valueRoleChanged FINAL REVISION(2, 14))
    // 2.15 (Qt 5.15)
    Q_PROPERTY(bool selectTextByMouse READ selectTextByMouse WRITE setSelectTextByMouse NOTIFY selectTextByMouseChanged FINAL REVISION(2, 15))
    // 6.0 (Qt 6.0)
    Q_PROPERTY(ImplicitContentWidthPolicy implicitContentWidthPolicy READ implicitContentWidthPolicy
        WRITE setImplicitContentWidthPolicy NOTIFY implicitContentWidthPolicyChanged FINAL REVISION(6, 0))
    QML_NAMED_ELEMENT(ComboBox)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickComboBox(QQuickItem *parent = nullptr);
    ~QQuickComboBox();

    int count() const;

    QVariant model() const;
    void setModel(const QVariant &model);
    QQmlInstanceModel *delegateModel() const;

    bool isPressed() const;
    void setPressed(bool pressed);

    int highlightedIndex() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    QString currentText() const;

    QString displayText() const;
    void setDisplayText(const QString &text);
    void resetDisplayText();

    QString textRole() const;
    void setTextRole(const QString &role);

    QString valueRole() const;
    void setValueRole(const QString &role);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

    QQuickPopup *popup() const;
    void setPopup(QQuickPopup *popup);

    Q_INVOKABLE QString textAt(int index) const;
    Q_INVOKABLE int find(const QString &text, Qt::MatchFlags flags = Qt::MatchExactly) const;

    // 2.1 (Qt 5.8)
    bool isFlat() const;
    void setFlat(bool flat);

    // 2.2 (Qt 5.9)
    bool isDown() const;
    void setDown(bool down);
    void resetDown();

    bool isEditable() const;
    void setEditable(bool editable);

    QString editText() const;
    void setEditText(const QString &text);
    void resetEditText();

#if QT_CONFIG(validator)
    QValidator *validator() const;
    void setValidator(QValidator *validator);
#endif

    Qt::InputMethodHints inputMethodHints() const;
    void setInputMethodHints(Qt::InputMethodHints hints);

    bool isInputMethodComposing() const;
    bool hasAcceptableInput() const;

    // 2.5 (Qt 5.12)
    qreal implicitIndicatorWidth() const;
    qreal implicitIndicatorHeight() const;

    // 2.14 (Qt 5.14)
    QVariant currentValue() const;
    Q_REVISION(6, 10) void setCurrentValue(const QVariant &value);
    Q_REVISION(2, 14) Q_INVOKABLE QVariant valueAt(int index) const;
    Q_REVISION(2, 14) Q_INVOKABLE int indexOfValue(const QVariant &value) const;

    // 2.15 (Qt 5.15)
    bool selectTextByMouse() const;
    void setSelectTextByMouse(bool canSelect);

    // 6.0 (Qt 6.0)
    enum ImplicitContentWidthPolicy {
        ContentItemImplicitWidth,
        WidestText,
        WidestTextWhenCompleted
    };
    Q_ENUM(ImplicitContentWidthPolicy)

    ImplicitContentWidthPolicy implicitContentWidthPolicy() const;
    void setImplicitContentWidthPolicy(ImplicitContentWidthPolicy policy);

public Q_SLOTS:
    void incrementCurrentIndex();
    void decrementCurrentIndex();
    Q_REVISION(2, 2) void selectAll();

Q_SIGNALS:
    void activated(int index);
    void highlighted(int index);
    void countChanged();
    void modelChanged();
    void delegateModelChanged();
    void pressedChanged();
    void highlightedIndexChanged();
    void currentIndexChanged();
    void currentTextChanged();
    void displayTextChanged();
    void textRoleChanged();
    void delegateChanged();
    void indicatorChanged();
    void popupChanged();
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void flatChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void accepted();
    Q_REVISION(2, 2) void downChanged();
    Q_REVISION(2, 2) void editableChanged();
    Q_REVISION(2, 2) void editTextChanged();
#if QT_CONFIG(validator)
    Q_REVISION(2, 2) void validatorChanged();
#endif
    Q_REVISION(2, 2) void inputMethodHintsChanged();
    Q_REVISION(2, 2) void inputMethodComposingChanged();
    Q_REVISION(2, 2) void acceptableInputChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void implicitIndicatorWidthChanged();
    Q_REVISION(2, 5) void implicitIndicatorHeightChanged();
    // 2.14 (Qt 5.14)
    Q_REVISION(2, 14) void valueRoleChanged();
    Q_REVISION(2, 14) void currentValueChanged();
    // 2.15 (Qt 5.15)
    Q_REVISION(2, 15) void selectTextByMouseChanged();
    // 6.0 (Qt 6.0)
    Q_REVISION(6, 0) void implicitContentWidthPolicyChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
#if QT_CONFIG(im)
    void inputMethodEvent(QInputMethodEvent *event) override;
#endif
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    bool event(QEvent *e) override;

    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void fontChange(const QFont &newFont, const QFont &oldFont) override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickComboBox)
    Q_DECLARE_PRIVATE(QQuickComboBox)
};

QT_END_NAMESPACE

#endif // QQUICKCOMBOBOX_P_H

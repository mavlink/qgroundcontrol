// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSPINBOX_P_H
#define QQUICKSPINBOX_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

class QValidator;
class QQuickSpinBoxPrivate;
class QQuickIndicatorButton;

class Q_QUICKTEMPLATES2_EXPORT QQuickSpinBox : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(int from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(int to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(int stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged FINAL)
    Q_PROPERTY(bool live READ isLive WRITE setLive NOTIFY liveChanged FINAL REVISION(6, 6))

#if QT_CONFIG(validator)
    Q_PROPERTY(QValidator *validator READ validator WRITE setValidator NOTIFY validatorChanged FINAL)
#endif
    Q_PROPERTY(QJSValue textFromValue READ textFromValue WRITE setTextFromValue NOTIFY textFromValueChanged FINAL)
    Q_PROPERTY(QJSValue valueFromText READ valueFromText WRITE setValueFromText NOTIFY valueFromTextChanged FINAL)
    Q_PROPERTY(QQuickIndicatorButton *up READ up CONSTANT FINAL)
    Q_PROPERTY(QQuickIndicatorButton *down READ down CONSTANT FINAL)
    // 2.2 (Qt 5.9)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged FINAL REVISION(2, 2))
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged FINAL REVISION(2, 2))
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool wrap READ wrap WRITE setWrap NOTIFY wrapChanged FINAL REVISION(2, 3))
    // 2.4 (Qt 5.11)
    Q_PROPERTY(QString displayText READ displayText NOTIFY displayTextChanged FINAL REVISION(2, 4))
    QML_NAMED_ELEMENT(SpinBox)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSpinBox(QQuickItem *parent = nullptr);
    ~QQuickSpinBox();

    int from() const;
    void setFrom(int from);

    int to() const;
    void setTo(int to);

    int value() const;
    void setValue(int value);

    int stepSize() const;
    void setStepSize(int step);

    bool isEditable() const;
    void setEditable(bool editable);

    bool isLive() const;
    void setLive(bool live);

#if QT_CONFIG(validator)
    QValidator *validator() const;
    void setValidator(QValidator *validator);
#endif

    QJSValue textFromValue() const;
    void setTextFromValue(const QJSValue &callback);

    QJSValue valueFromText() const;
    void setValueFromText(const QJSValue &callback);

    QQuickIndicatorButton *up() const;
    QQuickIndicatorButton *down() const;

    // 2.2 (Qt 5.9)
    Qt::InputMethodHints inputMethodHints() const;
    void setInputMethodHints(Qt::InputMethodHints hints);

    bool isInputMethodComposing() const;

    // 2.3 (Qt 5.10)
    bool wrap() const;
    void setWrap(bool wrap);

    // 2.4 (Qt 5.11)
    QString displayText() const;

public Q_SLOTS:
    void increase();
    void decrease();

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void valueChanged();
    void stepSizeChanged();
    void editableChanged();
    Q_REVISION(6, 6) void liveChanged();
#if QT_CONFIG(validator)
    void validatorChanged();
#endif
    void textFromValueChanged();
    void valueFromTextChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void valueModified();
    Q_REVISION(2, 2) void inputMethodHintsChanged();
    Q_REVISION(2, 2) void inputMethodComposingChanged();
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void wrapChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void displayTextChanged();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif

    void classBegin() override;
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickSpinBox)
    Q_DECLARE_PRIVATE(QQuickSpinBox)
};

QT_END_NAMESPACE

#endif // QQUICKSPINBOX_P_H

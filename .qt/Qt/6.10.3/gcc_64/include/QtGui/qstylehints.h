// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSTYLEHINTS_H
#define QSTYLEHINTS_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QPlatformIntegration;
class QStyleHintsPrivate;
class QAccessibilityHints;

class Q_GUI_EXPORT QStyleHints : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QStyleHints)
    Q_PROPERTY(int cursorFlashTime READ cursorFlashTime NOTIFY cursorFlashTimeChanged FINAL)
    Q_PROPERTY(qreal fontSmoothingGamma READ fontSmoothingGamma STORED false CONSTANT FINAL)
#if QT_DEPRECATED_SINCE(6, 5)
    Q_PROPERTY(int keyboardAutoRepeatRate READ keyboardAutoRepeatRate STORED false CONSTANT FINAL)
#endif
    Q_PROPERTY(qreal keyboardAutoRepeatRateF READ keyboardAutoRepeatRateF STORED false CONSTANT FINAL)
    Q_PROPERTY(int keyboardInputInterval READ keyboardInputInterval
               NOTIFY keyboardInputIntervalChanged FINAL)
    Q_PROPERTY(int mouseDoubleClickInterval READ mouseDoubleClickInterval
               NOTIFY mouseDoubleClickIntervalChanged FINAL)
    Q_PROPERTY(int mousePressAndHoldInterval READ mousePressAndHoldInterval
               NOTIFY mousePressAndHoldIntervalChanged FINAL)
    Q_PROPERTY(QChar passwordMaskCharacter READ passwordMaskCharacter STORED false CONSTANT FINAL)
    Q_PROPERTY(int passwordMaskDelay READ passwordMaskDelay STORED false CONSTANT FINAL)
    Q_PROPERTY(bool setFocusOnTouchRelease READ setFocusOnTouchRelease STORED false CONSTANT FINAL)
    Q_PROPERTY(bool showIsFullScreen READ showIsFullScreen STORED false CONSTANT FINAL)
    Q_PROPERTY(bool showIsMaximized READ showIsMaximized STORED false CONSTANT FINAL)
    Q_PROPERTY(bool showShortcutsInContextMenus READ showShortcutsInContextMenus
               WRITE setShowShortcutsInContextMenus NOTIFY showShortcutsInContextMenusChanged FINAL)
    Q_PROPERTY(Qt::ContextMenuTrigger contextMenuTrigger READ contextMenuTrigger WRITE
                   setContextMenuTrigger NOTIFY contextMenuTriggerChanged FINAL)
    Q_PROPERTY(int startDragDistance READ startDragDistance NOTIFY startDragDistanceChanged FINAL)
    Q_PROPERTY(int startDragTime READ startDragTime NOTIFY startDragTimeChanged FINAL)
    Q_PROPERTY(int startDragVelocity READ startDragVelocity STORED false CONSTANT FINAL)
    Q_PROPERTY(bool useRtlExtensions READ useRtlExtensions STORED false CONSTANT FINAL)
    Q_PROPERTY(Qt::TabFocusBehavior tabFocusBehavior READ tabFocusBehavior
               NOTIFY tabFocusBehaviorChanged FINAL)
    Q_PROPERTY(bool singleClickActivation READ singleClickActivation STORED false CONSTANT FINAL)
    Q_PROPERTY(bool useHoverEffects READ useHoverEffects WRITE setUseHoverEffects
               NOTIFY useHoverEffectsChanged FINAL)
    Q_PROPERTY(int wheelScrollLines READ wheelScrollLines NOTIFY wheelScrollLinesChanged FINAL)
    Q_PROPERTY(int mouseQuickSelectionThreshold READ mouseQuickSelectionThreshold
               WRITE setMouseQuickSelectionThreshold NOTIFY mouseQuickSelectionThresholdChanged
               FINAL)
    Q_PROPERTY(int mouseDoubleClickDistance READ mouseDoubleClickDistance STORED false CONSTANT
               FINAL)
    Q_PROPERTY(int touchDoubleTapDistance READ touchDoubleTapDistance STORED false CONSTANT FINAL)
    Q_PROPERTY(Qt::ColorScheme colorScheme READ colorScheme WRITE setColorScheme
               RESET unsetColorScheme NOTIFY colorSchemeChanged FINAL)
    Q_PROPERTY(bool menuSelectionWraps READ menuSelectionWraps STORED false CONSTANT FINAL REVISION(6, 10))
    Q_PROPERTY(const QAccessibilityHints* accessibility READ accessibility CONSTANT FINAL REVISION(6, 10))

public:
    void setMouseDoubleClickInterval(int mouseDoubleClickInterval);
    int mouseDoubleClickInterval() const;
    int mouseDoubleClickDistance() const;
    int touchDoubleTapDistance() const;
    void setMousePressAndHoldInterval(int mousePressAndHoldInterval);
    int mousePressAndHoldInterval() const;
    void setStartDragDistance(int startDragDistance);
    int startDragDistance() const;
    void setStartDragTime(int startDragTime);
    int startDragTime() const;
    int startDragVelocity() const;
    void setKeyboardInputInterval(int keyboardInputInterval);
    int keyboardInputInterval() const;
#if QT_DEPRECATED_SINCE(6, 5)
    QT_DEPRECATED_VERSION_X_6_5("Use keyboardAutoRepeatRateF() instead")
    int keyboardAutoRepeatRate() const;
#endif
    qreal keyboardAutoRepeatRateF() const;
    void setCursorFlashTime(int cursorFlashTime);
    int cursorFlashTime() const;
    bool showIsFullScreen() const;
    bool showIsMaximized() const;
    bool showShortcutsInContextMenus() const;
    void setShowShortcutsInContextMenus(bool showShortcutsInContextMenus);
    Qt::ContextMenuTrigger contextMenuTrigger() const;
    void setContextMenuTrigger(Qt::ContextMenuTrigger contextMenuTrigger);
    bool menuSelectionWraps() const;
    int passwordMaskDelay() const;
    QChar passwordMaskCharacter() const;
    qreal fontSmoothingGamma() const;
    bool useRtlExtensions() const;
    bool setFocusOnTouchRelease() const;
    Qt::TabFocusBehavior tabFocusBehavior() const;
    void setTabFocusBehavior(Qt::TabFocusBehavior tabFocusBehavior);
    bool singleClickActivation() const;
    bool useHoverEffects() const;
    void setUseHoverEffects(bool useHoverEffects);
    int wheelScrollLines() const;
    void setWheelScrollLines(int scrollLines);
    void setMouseQuickSelectionThreshold(int threshold);
    int mouseQuickSelectionThreshold() const;
    Qt::ColorScheme colorScheme() const;
    void setColorScheme(Qt::ColorScheme scheme);
    void unsetColorScheme() { setColorScheme(Qt::ColorScheme::Unknown); }
    const QAccessibilityHints* accessibility() const;

Q_SIGNALS:
    void cursorFlashTimeChanged(int cursorFlashTime);
    void keyboardInputIntervalChanged(int keyboardInputInterval);
    void mouseDoubleClickIntervalChanged(int mouseDoubleClickInterval);
    void mousePressAndHoldIntervalChanged(int mousePressAndHoldInterval);
    void startDragDistanceChanged(int startDragDistance);
    void startDragTimeChanged(int startDragTime);
    void tabFocusBehaviorChanged(Qt::TabFocusBehavior tabFocusBehavior);
    void useHoverEffectsChanged(bool useHoverEffects);
    void showShortcutsInContextMenusChanged(bool);
    void contextMenuTriggerChanged(Qt::ContextMenuTrigger contextMenuTrigger);
    void wheelScrollLinesChanged(int scrollLines);
    void mouseQuickSelectionThresholdChanged(int threshold);
    void colorSchemeChanged(Qt::ColorScheme colorScheme);

private:
    friend class QGuiApplication;
    QStyleHints();
};

QT_END_NAMESPACE

#endif

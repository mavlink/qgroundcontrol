#pragma once

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the critical vehicle message toast in MainWindow.qml.
///
/// The toast is a non-modal Popup layered over whatever view the user is in. It must never take
/// keyboard focus away from the user, and it must still be dismissable with Escape. Because the
/// popup has no focus, Popup.CloseOnEscape can never fire for it, so Escape is handled by a
/// Shortcut declared as a child of the popup. That placement is load bearing:
/// QQuickShortcutContext::matcher blocks any shortcut whose context item lives outside the topmost
/// CloseOnEscape popup, so a window-level Shortcut would silently never fire.
class CriticalVehicleMessageUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    /// The toast must not take keyboard focus from the MAVLink Console, typing must keep landing in
    /// the console while the toast is up, and Escape must dismiss it without disturbing the console.
    void _testPopupDoesNotStealMAVLinkConsoleFocus();

    /// Clicking the toast and pressing Escape must both run acknowledge() rather than a bare
    /// close(), so the pending "additional errors" state is consumed.
    void _testPopupAcknowledgedByClickAndEscape();

private:
    /// Activate the QML window, failing the test if it never becomes active. Shortcut matching
    /// requires an active window, so an inactive one would fail the Escape assertions for a reason
    /// unrelated to the code under test.
    bool _activateWindow();

    /// The critical message Popup. A Popup is not a QQuickItem, so it is found via findChild rather
    /// than the visual-tree helpers. Fails the test and returns nullptr if missing.
    QObject* _criticalMessagePopup();

    /// Call MainWindow.qml's showCriticalVehicleMessage() directly. Driving it through
    /// QGCApplication is not possible here: _showErrorsInToolbar is only set in
    /// _initForNormalAppBoot(), which unit tests never run.
    bool _showCriticalMessage(const QString& message);

    /// Wait until the popup reports opened (what gates the Escape Shortcut) and shows \a expectedMessage.
    bool _waitForPopupOpened(QObject* popup, const QString& expectedMessage);

    /// Wait until the popup is closed and its item has left the scene.
    bool _waitForPopupClosed(QObject* popup);

    /// Navigate Analyze -> MAVLink Console and return the page root.
    QQuickItem* _openMAVLinkConsolePage();

    /// Click the console TextArea and wait for it to take active focus. The page's own
    /// Component.onCompleted forceActiveFocus() never takes effect (it runs while the page is still
    /// parented to mainWindow, under two unfocused Loader focus scopes), so focus it as a user does.
    QQuickItem* _focusConsoleTextArea();

    /// Type ASCII text into the focused item, one key click at a time.
    void _typeText(const QString& text);

    /// The command typed at the console prompt. Uses the page's getCommand(), which excludes the
    /// prompt; textConsole.text is a serialized RichText document and useless for assertions.
    QString _consoleCommand(QQuickItem* consolePage);
};

#pragma once

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the critical vehicle message toast in MainWindow.qml.
class CriticalVehicleMessageUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    /// The toast must not take keyboard focus from the MAVLink Console.
    void _testPopupDoesNotStealMAVLinkConsoleFocus();

    /// Clicking the toast must acknowledge it, not just close it.
    void _testPopupAcknowledgedByClick();

    /// Stacked messages stop at the window bottom; the rest wait for the status drawer.
    void _testPopupStacksOnlyMessagesThatFit();

    /// The auto-dismiss timeout only closes the toast; it never acknowledges it.
    void _testTimeoutClosesWithoutAcknowledging();

    /// The overflow heading stays inside the popup's clickable bounds on narrow windows.
    void _testOverflowHeadingStaysInsidePopup();

private:
    /// Activate the QML window, failing the test if it never becomes active.
    bool _activateWindow();

    /// Fails the test and returns nullptr if the popup is missing.
    QObject* _criticalMessagePopup();

    /// Calls showCriticalVehicleMessage() directly; the QGCApplication path is off in unit tests.
    bool _showCriticalMessage(const QString& message);

    /// Wait until the popup reports opened and shows \a expectedMessage.
    bool _waitForPopupOpened(QObject* popup, const QString& expectedMessage);

    /// Wait until the popup is closed and its item has left the scene.
    bool _waitForPopupClosed(QObject* popup);

    /// Navigate Analyze -> MAVLink Console and return the page root.
    QQuickItem* _openMAVLinkConsolePage();

    /// Click the console TextArea to focus it; the page's own forceActiveFocus() never takes effect.
    QQuickItem* _focusConsoleTextArea();

    /// Type ASCII text into the focused item, one key click at a time.
    void _typeText(const QString& text);

    /// The typed command via getCommand(); textConsole.text is RichText and includes the prompt.
    QString _consoleCommand(QQuickItem* consolePage);
};

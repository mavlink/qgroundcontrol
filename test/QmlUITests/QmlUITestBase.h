#pragma once

#include "UnitTest.h"

#include <QtCore/QPointer>

#include <functional>

class MockLink;
class QQmlApplicationEngine;
class QQuickItem;
class QQuickWindow;
class QVariant;
class Vehicle;

/// Base class for QML UI smoke tests.
///
/// Provides engine boot/teardown and common click helpers so individual test
/// classes only contain the logic that is unique to them.
///
/// The base class overrides cleanup() to call stopUI() automatically, so the
/// QML engine and window are always destroyed even when a QVERIFY/QFAIL aborts
/// the test slot early.
///
/// Usage pattern:
/// \code
///   void MyTest::_testSomething()
///   {
///       startUI();
///       // ... test-specific code using _window, _rootItem, clickButton() ...
///       stopUI();   // optional: cleanup() will call it if omitted
///   }
/// \endcode
class QmlUITestBase : public UnitTest
{
    Q_OBJECT

protected slots:
    void cleanup() override
    {
        _verifyFileDialogTestHookConsumed();
        stopUI();
        UnitTest::cleanup();
    }

protected:
    /// Boot the full QML UI (MainWindow). Initialises subsystems, suppresses
    /// first-run prompts, registers image providers, and waits for the window
    /// to be exposed. Uses QVERIFY internally — check QTest::currentTestFailed()
    /// on return to propagate failures to the calling test slot.
    void startUI();

    /// Close the QML window and wait for it to settle.
    void closeUIWindow();

    /// Delete the QML engine and reset all pointers.
    /// Must be called after closeUIWindow() (or via stopUI()).
    void destroyUIEngine();

    /// Convenience: closeUIWindow() followed by destroyUIEngine().
    /// When a MockLink is active, call mockLink->disconnect() first so the
    /// window tears down with a null vehicle — this intentionally exposes
    /// null-reference bugs in QML bindings.
    void stopUI();

    /// Fail the current test if the QGCFileDialogController test hook is still
    /// armed at teardown. A hook armed but never consumed means the expected
    /// dialog never opened; left armed it would be silently consumed by the
    /// next dialog anywhere — possibly in a later test. Clears the hook so the
    /// failure stays in the offending test.
    void _verifyFileDialogTestHookConsumed();

    /// Finds a visible QQuickItem by objectName in the visual tree, polling with
    /// 50ms intervals up to timeoutMs. Returns nullptr if not found within the timeout.
    static QQuickItem *findVisibleItem(QQuickItem *root, const QString &objectName, int timeoutMs = 1000);

    /// Click the visible QQuickItem with \a objectName in the current window.
    /// Returns false if the item cannot be found.
    bool clickButton(const QString &objectName);

    /// Click the visible QQuickItem with \a objectName at a fractional position
    /// within the item ((0.5, 0.5) is the center). Returns false if the item
    /// cannot be found.
    bool clickItemFraction(const QString &objectName, qreal fractionX, qreal fractionY);

    /// Find an item that may live in a virtualized view (ListView/TableView/
    /// TreeView) inside the flickable with \a flickableObjectName. Virtualized
    /// delegates only exist near the viewport, so this steps the flickable
    /// through its content range until the item instantiates, then scrolls it
    /// into view. Returns nullptr if the item never appears.
    QQuickItem *findVisibleItemScrolled(const QString &objectName, const QString &flickableObjectName);

    /// Convenience: findVisibleItemScrolled() followed by clickButton().
    bool clickButtonScrolled(const QString &objectName, const QString &flickableObjectName);

    /// Open the toolbar Q-logo tool-select dropdown and click the entry with
    /// objectName \a viewObjectName (e.g. "toolbar_viewPlan", "toolbar_viewClose").
    /// Clicks the Q logo, waits up to \a timeoutMs for the entry to appear, then
    /// clicks it. Returns false (after recording a test failure) if any step fails.
    bool clickToolSelectDropdownButton(const QString &viewObjectName, int timeoutMs = 2000);

    /// Wait up to \a timeoutMs for a QGCPopupDialog whose title contains
    /// \a titleSubstring to become visible. Matches against the dialog title
    /// label (objectName "popupDialog_title"). Returns true once found.
    bool waitForDialog(const QString &titleSubstring, int timeoutMs = 3000);

    /// Returns true if a QGCPopupDialog whose title contains \a titleSubstring
    /// is currently visible. Does not wait — use to assert a dialog is absent.
    bool dialogVisible(const QString &titleSubstring);

    /// Wait up to \a timeoutMs for the popup dialog accept button to become
    /// visible, then click it. Returns false if the button never appears.
    bool acceptDialog(int timeoutMs = 3000);

    /// Wait up to \a timeoutMs for the popup dialog reject button to become
    /// visible, then click it. Returns false if the button never appears.
    bool rejectDialog(int timeoutMs = 3000);

    /// Verify the enabled state of a visible item found by \a objectName,
    /// waiting up to 2 seconds for bindings to settle. Returns false (after
    /// recording a test failure) if the item is missing or the enabled state
    /// does not match. \a context is prepended to failure messages.
    bool verifyEnabled(const QString &objectName, bool expectedEnabled, const QString &context);

    /// Verify the primary (highlight) state of a button found by \a objectName.
    /// Same semantics as verifyEnabled().
    bool verifyPrimary(const QString &objectName, bool expectedPrimary, const QString &context);

    /// Verify the checked state of a checkable button found by \a objectName.
    /// Same semantics as verifyEnabled().
    bool verifyChecked(const QString &objectName, bool expectedChecked, const QString &context);

    /// Same semantics as verifyEnabled().
    bool verifyText(const QString &objectName, const QString &expectedText, const QString &context);

    /// Verify an arbitrary property of a visible item found by \a objectName,
    /// waiting up to 2 seconds for bindings to settle. Fails the test if the
    /// item is missing, the property does not exist, or the value never matches.
    bool verifyProperty(const QString &objectName, const char *propertyName,
                        const QVariant &expectedValue, const QString &context);

    /// Verify that an item found by \a objectName is present-and-visible
    /// (\a expectedVisible true) or absent/hidden (false), waiting up to
    /// 2 seconds. Fails the test on mismatch.
    bool verifyVisibility(const QString &objectName, bool expectedVisible, const QString &context);

    /// Scroll the QQuickFlickable identified by \a flickableObjectName so that
    /// \a item's centre is fully visible inside the flickable. Does nothing if
    /// \a item is already visible or if the flickable cannot be found.
    void scrollIntoView(QQuickItem *item, const QString &flickableObjectName);

    /// Convenience wrapper: boots the UI, connects a MockLink, runs \a body
    /// with the active MockLink and Vehicle, then tears down in the correct
    /// order (disconnect → closeUIWindow → destroyUIEngine).
    ///
    /// \a body may use QVERIFY2/QFAIL; on failure the lambda returns early and
    /// teardown still runs via the scope guard.
    void runWithMockLink(
        const std::function<MockLink *()> &factory,
        const std::function<void(QPointer<MockLink>, Vehicle *)> &body);

    /// Disconnect a MockLink and wait for the active vehicle to clear.
    /// Safe to call with a null pointer. Always call before closeUIWindow() so
    /// QML handles a null vehicle while the window is still open, exposing
    /// binding bugs.
    void disconnectMockLink(QPointer<MockLink> mockLink);

    /// Register ignores for known warnings produced by any ArduPilot MockLink
    /// connection. Call once before connectMockLinkAndWaitReady().
    void ignoreAPMMockLinkWarnings();

    /// Start a MockLink using \a factory, wait for the vehicle to connect and
    /// parameters to be fully loaded, then return the MockLink pointer and set
    /// \a vehicleOut to the active Vehicle.
    ///
    /// Returns null and marks the test failed on any error — check the return
    /// value before proceeding. The caller owns teardown: call
    /// mockLink->disconnect(), closeUIWindow(), and destroyUIEngine() in that
    /// order — disconnecting first forces QML to handle a null vehicle while
    /// the window is still open, exposing binding bugs.
    QPointer<MockLink> connectMockLinkAndWaitReady(
        const std::function<MockLink *()> &factory,
        Vehicle *&vehicleOut);

    QQmlApplicationEngine *_engine   = nullptr;
    QQuickWindow          *_window   = nullptr;
    QQuickItem            *_rootItem = nullptr;
    int _viewDelay = 0;  ///< ms to pause between view switches when onscreen
    int _pageDelay = 0;  ///< ms to pause between page switches when onscreen

private:
    /// Shared implementation for the verify* helpers: find the visible item,
    /// guard against the property not existing (an invalid QVariant silently
    /// converts to false/"" which would make false/empty expectations pass
    /// vacuously), then wait up to 2 seconds for the property to match.
    /// Exposed publicly as verifyProperty().
    bool _verifyItemProperty(const QString &objectName, const char *propertyName,
                             const QVariant &expectedValue, const QString &context);
};

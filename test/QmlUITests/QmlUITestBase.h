#pragma once

#include "UnitTest.h"

#include <QtCore/QPointer>

#include <functional>

class MockLink;
class QQmlApplicationEngine;
class QQuickItem;
class QQuickWindow;
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

    /// Click the visible QQuickItem with \a objectName in the current window.
    /// Returns false if the item cannot be found.
    bool clickButton(const QString &objectName);

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
};

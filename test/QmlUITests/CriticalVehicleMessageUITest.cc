#include "CriticalVehicleMessageUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QVariant>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"

UT_REGISTER_TEST(CriticalVehicleMessageUITest, TestLabel::Integration)

namespace {

constexpr const char* kPopupObjectName = "criticalVehicleMessage_popup";
constexpr const char* kMessageTextObjectName = "criticalVehicleMessage_text";
constexpr const char* kConsolePageObjectName = "mavlinkConsole_page";
constexpr const char* kConsoleTextAreaObjectName = "mavlinkConsole_textArea";
constexpr const char* kConsoleButtonObjectName = "analyzeButton_MAVLink Console";
constexpr const char* kIndicatorDrawerObjectName = "indicatorDrawerLoader";

constexpr const char* kFirstMessage = "Test critical vehicle message";
constexpr const char* kSecondMessage = "Second critical vehicle message";
constexpr const char* kThirdMessage = "Third critical vehicle message";

// Split so the second half is typed while the toast is on screen: the console must receive it even
// though a popup is open above it.
constexpr const char* kCommandFirstHalf = "ver";
constexpr const char* kCommandSecondHalf = "sion";
constexpr const char* kCommandFull = "version";

QString objName(const char* name)
{
    return QString::fromLatin1(name);
}

}  // namespace

bool CriticalVehicleMessageUITest::_activateWindow()
{
    if (!_window) {
        QTest::qFail("No QML window", __FILE__, __LINE__);
        return false;
    }

    // The offscreen QPA activates a window on show, but request it explicitly and fail loudly
    // otherwise: an inactive window disables every shortcut in the process, which would fail the
    // Escape assertions below for a reason that has nothing to do with the code under test.
    _window->requestActivate();
    if (!QTest::qWaitForWindowActive(_window, TestTimeout::mediumMs())) {
        QTest::qFail("QML window never became active; Shortcut handling requires an active window", __FILE__, __LINE__);
        return false;
    }
    return true;
}

QObject* CriticalVehicleMessageUITest::_criticalMessagePopup()
{
    // A Popup is not a QQuickItem, so the visual-tree helpers cannot see it. It survives close()
    // (only its popupItem leaves the scene), so the pointer stays valid for the whole test.
    QObject* const popup = _window ? _window->findChild<QObject*>(objName(kPopupObjectName)) : nullptr;
    if (!popup) {
        QTest::qFail("criticalVehicleMessage_popup not found in MainWindow", __FILE__, __LINE__);
    }
    return popup;
}

bool CriticalVehicleMessageUITest::_showCriticalMessage(const QString& message)
{
    QVariant returnedValue;
    if (!QMetaObject::invokeMethod(_window, "showCriticalVehicleMessage", Q_RETURN_ARG(QVariant, returnedValue),
                                   Q_ARG(QVariant, QVariant::fromValue(message)))) {
        QTest::qFail("showCriticalVehicleMessage() invocation failed", __FILE__, __LINE__);
        return false;
    }
    return true;
}

bool CriticalVehicleMessageUITest::_waitForPopupOpened(QObject* popup, const QString& expectedMessage)
{
    // `opened`, not `visible`, is what gates the Escape Shortcut.
    if (!waitForCondition([popup] { return popup->property("opened").toBool(); }, TestTimeout::mediumMs(),
                          QStringLiteral("critical vehicle message popup opened"))) {
        QTest::qFail("critical vehicle message popup never opened", __FILE__, __LINE__);
        return false;
    }
    if (!findVisibleItem(_rootItem, objName(kMessageTextObjectName), TestTimeout::shortMs())) {
        QTest::qFail("critical vehicle message popup opened but its label is not in the visible item tree", __FILE__,
                     __LINE__);
        return false;
    }
    return verifyText(objName(kMessageTextObjectName), expectedMessage, QStringLiteral("critical message toast"));
}

bool CriticalVehicleMessageUITest::_waitForPopupClosed(QObject* popup)
{
    if (!waitForCondition(
            [popup] { return !popup->property("visible").toBool() && !popup->property("opened").toBool(); },
            TestTimeout::mediumMs(), QStringLiteral("critical vehicle message popup closed"))) {
        return false;
    }
    // finalizeExitTransition() detaches popupItem from the scene before flipping `visible`, so once
    // the wait above returns the item is already gone - ordering-safe, not a race.
    if (findItem(_rootItem, objName(kMessageTextObjectName)) != nullptr) {
        QTest::qFail("critical vehicle message popup closed but its item is still parented into the scene", __FILE__,
                     __LINE__);
        return false;
    }
    return true;
}

QQuickItem* CriticalVehicleMessageUITest::_openMAVLinkConsolePage()
{
    if (!clickToolSelectDropdownButton(QStringLiteral("toolbar_viewAnalyze"))) {
        return nullptr;
    }
    if (!clickButton(objName(kConsoleButtonObjectName))) {
        QTest::qFail("MAVLink Console analyze page button not found", __FILE__, __LINE__);
        return nullptr;
    }
    QQuickItem* const page = findVisibleItem(_rootItem, objName(kConsolePageObjectName), TestTimeout::mediumMs());
    if (!page) {
        QTest::qFail("MAVLink Console page never loaded", __FILE__, __LINE__);
    }
    return page;
}

QQuickItem* CriticalVehicleMessageUITest::_focusConsoleTextArea()
{
    QQuickItem* const textArea =
        findVisibleItem(_rootItem, objName(kConsoleTextAreaObjectName), TestTimeout::mediumMs());
    if (!textArea) {
        QTest::qFail("MAVLink Console text area not found", __FILE__, __LINE__);
        return nullptr;
    }
    if (!_clickItemAt(textArea, 0.5, 0.5, objName(kConsoleTextAreaObjectName))) {
        return nullptr;
    }
    if (!waitForCondition([textArea] { return textArea->hasActiveFocus(); }, TestTimeout::shortMs(),
                          QStringLiteral("MAVLink Console text area focused"))) {
        QTest::qFail("MAVLink Console text area never took keyboard focus", __FILE__, __LINE__);
        return nullptr;
    }
    return textArea;
}

void CriticalVehicleMessageUITest::_typeText(const QString& text)
{
    for (const QChar& c : text) {
        QTest::keyClick(_window, c.toLatin1());
    }
}

QString CriticalVehicleMessageUITest::_consoleCommand(QQuickItem* consolePage)
{
    // Deliberately does not fail the test: callers poll this from QTRY_COMPARE, and a missing
    // getCommand() surfaces as an obvious empty-vs-expected diff.
    QVariant command;
    (void) QMetaObject::invokeMethod(consolePage, "getCommand", Q_RETURN_ARG(QVariant, command));
    return command.toString();
}

void CriticalVehicleMessageUITest::_testPopupDoesNotStealMAVLinkConsoleFocus()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
            QVERIFY(_activateWindow());

            // The console page is requiresVehicle; the mock link connected by runWithMockLink satisfies it.
            QQuickItem* const consolePage = _openMAVLinkConsolePage();
            QVERIFY(consolePage);

            QQuickItem* const textArea = _focusConsoleTextArea();
            QVERIFY(textArea);

            // Start typing a command. Never press Enter: Keys.onPressed would call getCommandAndClear()
            // and delete everything typed so far.
            _typeText(objName(kCommandFirstHalf));
            QTRY_COMPARE_WITH_TIMEOUT(_consoleCommand(consolePage), objName(kCommandFirstHalf), TestTimeout::shortMs());

            QObject* const popup = _criticalMessagePopup();
            QVERIFY(popup);
            QVERIFY2(!popup->property("visible").toBool(),
                     "critical message toast was already open before the test opened it");

            // A vehicle error arrives mid-typing.
            QVERIFY(_showCriticalMessage(objName(kFirstMessage)));
            QVERIFY(_waitForPopupOpened(popup, objName(kFirstMessage)));

            // THE REGRESSION: the toast is non-modal and must not take focus.
            QQuickItem* const focusItem = _window->activeFocusItem();
            const QString focusName = focusItem ? (focusItem->objectName().isEmpty()
                                                       ? QString::fromLatin1(focusItem->metaObject()->className())
                                                       : focusItem->objectName())
                                                : QStringLiteral("<null>");
            QVERIFY2(textArea->hasActiveFocus(),
                     qPrintable(QStringLiteral("critical message toast stole keyboard focus from the MAVLink "
                                               "Console (active focus item is now '%1')")
                                    .arg(focusName)));

            // ... and keystrokes must keep reaching the console while it is up.
            _typeText(objName(kCommandSecondHalf));
            QTRY_COMPARE_WITH_TIMEOUT(_consoleCommand(consolePage), objName(kCommandFull), TestTimeout::shortMs());

            // Escape dismisses the toast even though it never had focus. This only works because the
            // Shortcut is declared inside the popup.
            QTest::keyClick(_window, Qt::Key_Escape);
            QVERIFY2(_waitForPopupClosed(popup), "Escape did not close the critical message toast");

            // Dismissing must not disturb the console: same focus, same typed command.
            QVERIFY2(textArea->hasActiveFocus(), "MAVLink Console lost keyboard focus when the toast was dismissed");
            QCOMPARE(_consoleCommand(consolePage), objName(kCommandFull));
        });
}

void CriticalVehicleMessageUITest::_testPopupAcknowledgedByClickAndEscape()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
            QVERIFY(_activateWindow());

            // Stay on the Fly view: acknowledging with additional messages pending drops the Fly
            // toolbar's main status indicator, which is the observable proof that acknowledge() ran.
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), TestTimeout::mediumMs()),
                     "Fly view not visible");

            QObject* const popup = _criticalMessagePopup();
            QVERIFY(popup);

            // Clicking the toast dismisses it.
            QVERIFY(_showCriticalMessage(objName(kFirstMessage)));
            QVERIFY(_waitForPopupOpened(popup, objName(kFirstMessage)));
            QVERIFY2(!popup->property("additionalCriticalMessagesReceived").toBool(),
                     "additionalCriticalMessagesReceived set by a single message");

            // Clicks the label, which sits under the popup's full-surface MouseArea.
            QVERIFY2(clickButton(objName(kMessageTextObjectName)), "could not click the critical message toast");
            QVERIFY2(_waitForPopupClosed(popup), "clicking the critical message toast did not close it");

            // Escape must run acknowledge(), not a bare close().
            QVERIFY(_showCriticalMessage(objName(kSecondMessage)));
            QVERIFY(_waitForPopupOpened(popup, objName(kSecondMessage)));

            // A further error while the toast is up only raises the "additional errors" flag; the
            // displayed message stays the first one.
            QVERIFY(_showCriticalMessage(objName(kThirdMessage)));
            QTRY_VERIFY2_WITH_TIMEOUT(popup->property("additionalCriticalMessagesReceived").toBool(),
                                      "second message did not set additionalCriticalMessagesReceived",
                                      TestTimeout::shortMs());
            QVERIFY(verifyText(objName(kMessageTextObjectName), objName(kSecondMessage),
                               QStringLiteral("toast text after a second message")));

            QTest::keyClick(_window, Qt::Key_Escape);
            QVERIFY2(_waitForPopupClosed(popup), "Escape did not close the critical message toast");

            // acknowledge() consumes the flag and drops the main status indicator. A plain close()
            // would leave both untouched.
            QVERIFY2(!popup->property("additionalCriticalMessagesReceived").toBool(),
                     "Escape closed the toast without acknowledging it: additionalCriticalMessagesReceived still set");
            QVERIFY2(findVisibleItem(_rootItem, objName(kIndicatorDrawerObjectName), TestTimeout::mediumMs()),
                     "acknowledging with additional messages pending did not drop the main status indicator tool");

            // Close the indicator drawer so teardown starts from a clean UI.
            QTest::keyClick(_window, Qt::Key_Escape);
            QVERIFY2(
                waitForCondition(
                    [this] { return findVisibleItem(_rootItem, objName(kIndicatorDrawerObjectName), 0) == nullptr; },
                    TestTimeout::mediumMs(), QStringLiteral("main status indicator drawer closed")),
                "main status indicator drawer did not close on Escape");
        });
}

#include "CriticalVehicleMessageUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QtMath>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"

UT_REGISTER_TEST(CriticalVehicleMessageUITest, TestLabel::Integration)

namespace {

constexpr const char* kPopupObjectName = "criticalVehicleMessage_popup";
constexpr const char* kMessageTextObjectName = "criticalVehicleMessage_text";
constexpr const char* kHeadingObjectName = "criticalVehicleMessage_heading";
constexpr const char* kDismissTimerObjectName = "criticalVehicleMessage_dismissTimer";
constexpr const char* kConsolePageObjectName = "mavlinkConsole_page";
constexpr const char* kConsoleTextAreaObjectName = "mavlinkConsole_textArea";
constexpr const char* kConsoleButtonObjectName = "analyzeButton_MAVLink Console";
constexpr const char* kIndicatorDrawerObjectName = "indicatorDrawerLoader";

constexpr const char* kFirstMessage = "Test critical vehicle message";
constexpr int kMaxShownMessages = 5;

// The second half is typed while the toast is on screen.
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

    // Fail loudly if the window never activates so focus assertions aren't confounded by it.
    _window->requestActivate();
    if (!QTest::qWaitForWindowActive(_window, TestTimeout::mediumMs())) {
        QTest::qFail("QML window never became active; focus assertions need an active window", __FILE__, __LINE__);
        return false;
    }
    return true;
}

QObject* CriticalVehicleMessageUITest::_criticalMessagePopup()
{
    // Not a QQuickItem, so the visual-tree helpers can't find it; it outlives close().
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
    // popupItem leaves the scene before `visible` flips, so this check isn't racy.
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
    // Doesn't fail the test: callers poll this from QTRY_COMPARE.
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

            QQuickItem* const consolePage = _openMAVLinkConsolePage();
            QVERIFY(consolePage);

            QQuickItem* const textArea = _focusConsoleTextArea();
            QVERIFY(textArea);

            // Never press Enter: it submits and clears the command.
            _typeText(objName(kCommandFirstHalf));
            QTRY_COMPARE_WITH_TIMEOUT(_consoleCommand(consolePage), objName(kCommandFirstHalf), TestTimeout::shortMs());

            QObject* const popup = _criticalMessagePopup();
            QVERIFY(popup);
            QVERIFY2(!popup->property("visible").toBool(),
                     "critical message toast was already open before the test opened it");

            // A vehicle error arrives mid-typing.
            QVERIFY(_showCriticalMessage(objName(kFirstMessage)));
            QVERIFY(_waitForPopupOpened(popup, objName(kFirstMessage)));

            QQuickItem* const focusItem = _window->activeFocusItem();
            const QString focusName = focusItem ? (focusItem->objectName().isEmpty()
                                                       ? QString::fromLatin1(focusItem->metaObject()->className())
                                                       : focusItem->objectName())
                                                : QStringLiteral("<null>");
            QVERIFY2(textArea->hasActiveFocus(),
                     qPrintable(QStringLiteral("critical message toast stole keyboard focus from the MAVLink "
                                               "Console (active focus item is now '%1')")
                                    .arg(focusName)));

            _typeText(objName(kCommandSecondHalf));
            QTRY_COMPARE_WITH_TIMEOUT(_consoleCommand(consolePage), objName(kCommandFull), TestTimeout::shortMs());

            QVERIFY2(clickButton(objName(kMessageTextObjectName)), "could not click the critical message toast");
            QVERIFY2(_waitForPopupClosed(popup), "clicking the critical message toast did not close it");

            QVERIFY2(textArea->hasActiveFocus(), "MAVLink Console lost keyboard focus when the toast was dismissed");
            QCOMPARE(_consoleCommand(consolePage), objName(kCommandFull));
        });
}

void CriticalVehicleMessageUITest::_testPopupAcknowledgedByClick()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
            QVERIFY(_activateWindow());

            // The main status indicator drawer is on the Fly view toolbar.
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), TestTimeout::mediumMs()),
                     "Fly view not visible");

            QObject* const popup = _criticalMessagePopup();
            QVERIFY(popup);

            QVERIFY(_showCriticalMessage(objName(kFirstMessage)));
            QVERIFY(_waitForPopupOpened(popup, objName(kFirstMessage)));
            QVERIFY2(!popup->property("additionalCriticalMessagesReceived").toBool(),
                     "additionalCriticalMessagesReceived set by a single message");

            QVERIFY2(clickButton(objName(kMessageTextObjectName)), "could not click the critical message toast");
            QVERIFY2(_waitForPopupClosed(popup), "clicking the critical message toast did not close it");

            QStringList stackedMessages;
            for (int i = 1; i <= kMaxShownMessages + 1; i++) {
                stackedMessages.append(QStringLiteral("Stacked critical vehicle message %1").arg(i));
            }

            QVERIFY(_showCriticalMessage(stackedMessages.first()));
            QVERIFY(_waitForPopupOpened(popup, stackedMessages.first()));

            // Errors arriving while the toast is up are added to it, up to the limit.
            for (int i = 1; i < kMaxShownMessages; i++) {
                QVERIFY(_showCriticalMessage(stackedMessages[i]));
            }
            const QString shownText = stackedMessages.mid(0, kMaxShownMessages).join(QStringLiteral("<br/>"));
            QVERIFY(verifyText(objName(kMessageTextObjectName), shownText,
                               QStringLiteral("toast text with stacked messages")));
            QVERIFY2(!popup->property("additionalCriticalMessagesReceived").toBool(),
                     "additionalCriticalMessagesReceived set before the toast was full");

            // Beyond the limit only the flag is set; the text doesn't change.
            QVERIFY(_showCriticalMessage(stackedMessages.last()));
            QTRY_VERIFY2_WITH_TIMEOUT(popup->property("additionalCriticalMessagesReceived").toBool(),
                                      "message beyond the limit did not set additionalCriticalMessagesReceived",
                                      TestTimeout::shortMs());
            QVERIFY(verifyText(objName(kMessageTextObjectName), shownText,
                               QStringLiteral("toast text after a message beyond the limit")));

            // The "Click to see more" heading straddles the popup's top edge: its top half must acknowledge too
            QQuickItem* const heading = findVisibleItem(_rootItem, objName(kHeadingObjectName));
            QVERIFY2(heading, "critical message toast heading not visible");
            QVERIFY(_clickItemAt(heading, 0.5, 0.25, objName(kHeadingObjectName)));
            QVERIFY2(_waitForPopupClosed(popup), "clicking the critical message toast heading did not close it");

            QVERIFY2(!popup->property("additionalCriticalMessagesReceived").toBool(),
                     "click closed the toast without acknowledging it: additionalCriticalMessagesReceived still set");
            QVERIFY2(findVisibleItem(_rootItem, objName(kIndicatorDrawerObjectName), TestTimeout::mediumMs()),
                     "acknowledging with additional messages pending did not drop the main status indicator tool");

            QTest::keyClick(_window, Qt::Key_Escape);
            QVERIFY2(
                waitForCondition(
                    [this] { return findVisibleItem(_rootItem, objName(kIndicatorDrawerObjectName), 0) == nullptr; },
                    TestTimeout::mediumMs(), QStringLiteral("main status indicator drawer closed")),
                "main status indicator drawer did not close on Escape");
        });
}

void CriticalVehicleMessageUITest::_testPopupStacksOnlyMessagesThatFit()
{
    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QObject* const popup = _criticalMessagePopup();
    QVERIFY(popup);

    QStringList messages;
    for (int i = 1; i <= kMaxShownMessages; i++) {
        messages.append(QStringLiteral("Height limited critical vehicle message %1").arg(i));
    }

    // A window twice the single-message popup's height has room for a second message, but not for all of them
    QVERIFY(_showCriticalMessage(messages.first()));
    QVERIFY(_waitForPopupOpened(popup, messages.first()));
    const int windowHeight = qCeil(popup->property("y").toReal() + (2.0 * popup->property("height").toReal()));
    QVERIFY(QMetaObject::invokeMethod(popup, "close"));
    QVERIFY(_waitForPopupClosed(popup));
    QCOMPARE_LT(windowHeight, _window->height());
    // MainWindowSavedState sets a minimum height that onscreen platforms enforce
    _window->setMinimumHeight(0);
    _window->resize(_window->width(), windowHeight);
    QTRY_COMPARE_WITH_TIMEOUT(_rootItem->height(), windowHeight, TestTimeout::shortMs());

    QVERIFY(_showCriticalMessage(messages.first()));
    QVERIFY(_waitForPopupOpened(popup, messages.first()));
    for (int i = 1; i < kMaxShownMessages; i++) {
        QVERIFY(_showCriticalMessage(messages[i]));
    }

    const int shownCount = popup->property("shownMessageCount").toInt();
    QCOMPARE_GT(shownCount, 1);
    QCOMPARE_LT(shownCount, kMaxShownMessages);
    QVERIFY(verifyText(objName(kMessageTextObjectName), messages.mid(0, shownCount).join(QStringLiteral("<br/>")),
                       QStringLiteral("toast text limited by window height")));
    QVERIFY2(popup->property("additionalCriticalMessagesReceived").toBool(),
             "messages that did not fit did not set additionalCriticalMessagesReceived");
    QCOMPARE_LE(popup->property("y").toReal() + popup->property("height").toReal(), _window->height());

    stopUI();
}

void CriticalVehicleMessageUITest::_testTimeoutClosesWithoutAcknowledging()
{
    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QObject* const popup = _criticalMessagePopup();
    QVERIFY(popup);
    QObject* const dismissTimer = popup->findChild<QObject*>(objName(kDismissTimerObjectName));
    QVERIFY2(dismissTimer, "critical message dismiss timer not found");
    QVERIFY(dismissTimer->setProperty("interval", 500));

    // Overflow pending: acknowledging would drop the main status indicator drawer
    for (int i = 1; i <= kMaxShownMessages + 1; i++) {
        QVERIFY(_showCriticalMessage(QStringLiteral("Timed out critical vehicle message %1").arg(i)));
    }
    QVERIFY(popup->property("visible").toBool());
    QVERIFY(popup->property("additionalCriticalMessagesReceived").toBool());

    QVERIFY2(_waitForPopupClosed(popup), "critical message toast did not time out");
    QVERIFY2(popup->property("additionalCriticalMessagesReceived").toBool(),
             "timeout acknowledged the toast: additionalCriticalMessagesReceived cleared");
    QVERIFY2(!findVisibleItem(_rootItem, objName(kIndicatorDrawerObjectName), 0),
             "timeout dropped the main status indicator drawer");

    stopUI();
}

void CriticalVehicleMessageUITest::_testOverflowHeadingStaysInsidePopup()
{
    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QObject* const popup = _criticalMessagePopup();
    QVERIFY(popup);
    for (int i = 1; i <= kMaxShownMessages + 1; i++) {
        QVERIFY(_showCriticalMessage(QStringLiteral("Overflow critical vehicle message %1").arg(i)));
    }
    QVERIFY(popup->property("additionalCriticalMessagesReceived").toBool());
    QQuickItem* const heading = findVisibleItem(_rootItem, objName(kHeadingObjectName));
    QVERIFY2(heading, "critical message toast heading not visible");

    // Narrow the window until the popup (55% of it) is narrower than the full overflow heading
    const int windowWidth = qCeil((heading->width() * 0.8) / 0.55);
    QCOMPARE_LT(windowWidth, _window->width());
    // MainWindowSavedState sets a minimum width that onscreen platforms enforce
    _window->setMinimumWidth(0);
    _window->resize(windowWidth, _window->height());
    QTRY_COMPARE_WITH_TIMEOUT(_rootItem->width(), windowWidth, TestTimeout::shortMs());

    // Outside the popup a click would close it without opening the status drawer
    const QQuickItem* const background = heading->parentItem();
    QVERIFY(background);
    QCOMPARE_GE(heading->x(), 0.0);
    QCOMPARE_LE(heading->x() + heading->width(), background->width());

    stopUI();
}

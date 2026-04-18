#include "MAVLinkConsoleControllerTest.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

#include "MAVLinkConsoleController.h"

// ---------------------------------------------------------------------------
// Construction / initial state
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_constructionTest()
{
    // Should construct and destruct without crashing even with no active vehicle.
    MAVLinkConsoleController controller;
    QVERIFY(true);
}

void MAVLinkConsoleControllerTest::_initialStateTest()
{
    MAVLinkConsoleController controller;

    // Model must start empty.
    QCOMPARE(controller.rowCount(), 0);

    // text property must return an empty string when there are no rows.
    const QString text = controller.property("text").toString();
    QCOMPARE(text, QString());
}

// ---------------------------------------------------------------------------
// CommandHistory – navigation on empty history
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_historyUpOnEmptyTest()
{
    MAVLinkConsoleController controller;

    // With no history, historyUp must echo back whatever was passed in.
    const QString current = QStringLiteral("my input");
    QCOMPARE(controller.historyUp(current), current);
}

void MAVLinkConsoleControllerTest::_historyDownOnEmptyTest()
{
    MAVLinkConsoleController controller;

    // With no history, historyDown must echo back whatever was passed in.
    const QString current = QStringLiteral("my input");
    QCOMPARE(controller.historyDown(current), current);
}

// ---------------------------------------------------------------------------
// CommandHistory – single entry
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_historyUpSingleEntryTest()
{
    MAVLinkConsoleController controller;

    // sendCommand appends non-empty lines to history even without a vehicle.
    controller.sendCommand(QStringLiteral("ls"));

    // First up should yield the recorded command.
    QCOMPARE(controller.historyUp(QStringLiteral("")), QStringLiteral("ls"));

    // A second up at the top boundary must return the same current string.
    QCOMPARE(controller.historyUp(QStringLiteral("ls")), QStringLiteral("ls"));
}

// ---------------------------------------------------------------------------
// CommandHistory – multi-entry up/down round-trip
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_historyUpDownNavigationTest()
{
    MAVLinkConsoleController controller;

    controller.sendCommand(QStringLiteral("cmd1"));
    controller.sendCommand(QStringLiteral("cmd2"));
    controller.sendCommand(QStringLiteral("cmd3"));

    // Navigate backwards: cmd3 -> cmd2 -> cmd1
    const QString r1 = controller.historyUp(QStringLiteral(""));
    QCOMPARE(r1, QStringLiteral("cmd3"));

    const QString r2 = controller.historyUp(r1);
    QCOMPARE(r2, QStringLiteral("cmd2"));

    const QString r3 = controller.historyUp(r2);
    QCOMPARE(r3, QStringLiteral("cmd1"));

    // Navigate forward: cmd2 -> cmd3 -> empty (past end)
    const QString f1 = controller.historyDown(r3);
    QCOMPARE(f1, QStringLiteral("cmd2"));

    const QString f2 = controller.historyDown(f1);
    QCOMPARE(f2, QStringLiteral("cmd3"));

    const QString f3 = controller.historyDown(f2);
    QCOMPARE(f3, QStringLiteral(""));
}

// ---------------------------------------------------------------------------
// CommandHistory – duplicate suppression
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_historyNoDuplicatesTest()
{
    MAVLinkConsoleController controller;

    controller.sendCommand(QStringLiteral("ping"));
    controller.sendCommand(QStringLiteral("ping"));  // duplicate – must not be appended

    // A single up should reach "ping".
    const QString r1 = controller.historyUp(QStringLiteral(""));
    QCOMPARE(r1, QStringLiteral("ping"));

    // Another up at the boundary must return the same string (only one entry).
    const QString r2 = controller.historyUp(r1);
    QCOMPARE(r2, QStringLiteral("ping"));
}

// ---------------------------------------------------------------------------
// CommandHistory – boundary: already at top (index 0)
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_historyUpAtTopBoundaryTest()
{
    MAVLinkConsoleController controller;

    controller.sendCommand(QStringLiteral("alpha"));

    // Move to top.
    controller.historyUp(QStringLiteral(""));

    // Another up must return current unchanged (boundary protection).
    const QString current = QStringLiteral("alpha");
    QCOMPARE(controller.historyUp(current), current);
}

// ---------------------------------------------------------------------------
// CommandHistory – boundary: already at bottom (index == history.length())
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_historyDownAtBottomBoundaryTest()
{
    MAVLinkConsoleController controller;

    controller.sendCommand(QStringLiteral("beta"));

    // At the bottom (fresh after sendCommand), down must echo current.
    const QString current = QStringLiteral("beta");
    QCOMPARE(controller.historyDown(current), current);
}

// ---------------------------------------------------------------------------
// handleClipboard – no newline in combined string
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_handleClipboardNoNewlineTest()
{
    MAVLinkConsoleController controller;

    // Clear clipboard so the result is purely from command_pre.
    QGuiApplication::clipboard()->clear();

    const QString prefix = QStringLiteral("prefix");
    const QString result = controller.handleClipboard(prefix);

    // No newline present, so the whole string is returned as the pending line.
    QCOMPARE(result, prefix);
}

// ---------------------------------------------------------------------------
// handleClipboard – newline at the end of clipboard text
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_handleClipboardWithNewlineTest()
{
    MAVLinkConsoleController controller;

    // Place a newline-terminated string in the clipboard.
    QGuiApplication::clipboard()->setText(QStringLiteral("clipped\n"));

    // With an empty prefix the clipboard text has a trailing newline.
    // The part before the newline is sent as a command; the remainder
    // (empty string after the newline) is returned.
    const QString result = controller.handleClipboard(QStringLiteral(""));
    QCOMPARE(result, QStringLiteral(""));
}

// ---------------------------------------------------------------------------
// handleClipboard – empty prefix, clipboard with no newline
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_handleClipboardEmptyPrefixTest()
{
    MAVLinkConsoleController controller;

    QGuiApplication::clipboard()->setText(QStringLiteral("data"));

    // No newline -> entire combined string returned, nothing sent.
    const QString result = controller.handleClipboard(QStringLiteral(""));
    QCOMPARE(result, QStringLiteral("data"));
}

// ---------------------------------------------------------------------------
// handleClipboard – multiple newlines, last line returned
// ---------------------------------------------------------------------------

void MAVLinkConsoleControllerTest::_handleClipboardMultilineTest()
{
    MAVLinkConsoleController controller;

    // Three lines: "a\nb\nc" — last line has no trailing newline.
    QGuiApplication::clipboard()->setText(QStringLiteral("a\nb\nc"));

    // Combined string is "a\nb\nc". Last newline is between "b" and "c".
    // "a\nb" is sent; "c" is returned.
    const QString result = controller.handleClipboard(QStringLiteral(""));
    QCOMPARE(result, QStringLiteral("c"));
}

UT_REGISTER_TEST(MAVLinkConsoleControllerTest, TestLabel::Unit, TestLabel::AnalyzeView)

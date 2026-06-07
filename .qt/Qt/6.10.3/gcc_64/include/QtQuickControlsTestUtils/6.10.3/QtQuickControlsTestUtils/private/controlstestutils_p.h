// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CONTROLSTESTUTILS_P_H
#define CONTROLSTESTUTILS_P_H

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

#include <QtGui/qpa/qplatformtheme.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQmlEngine;
class QQuickApplicationWindow;
class QQuickAbstractButton;
class QQuickControl;

namespace QQuickControlsTestUtils
{
    class QQuickControlsApplicationHelper : public QQuickVisualTestUtils::QQuickApplicationHelper
    {
    public:
        QQuickControlsApplicationHelper(QQmlDataTest *testCase, const QString &testFilePath,
                const QVariantMap &initialProperties = {},
                const QStringList &qmlImportPaths = {});

        QQuickApplicationWindow *appWindow = nullptr;
    };

    struct QQuickStyleHelper
    {
        [[nodiscard]] bool updateStyle(const QString &style);

        QString currentStyle;
        QScopedPointer<QQmlEngine> engine;
    };

    typedef std::function<void(const QString &/*relativePath*/, const QUrl &/*absoluteUrl*/)> ForEachCallback;

    void forEachControl(QQmlEngine *engine, const QString &qqc2ImportPath, const QString &sourcePath,
        const QString &targetPath, const QStringList &skipList, ForEachCallback callback);
    void addTestRowForEachControl(QQmlEngine *engine, const QString &qqc2ImportPath, const QString &sourcePath,
        const QString &targetPath, const QStringList &skipList = QStringList());

    [[nodiscard]] bool verifyButtonClickable(QQuickAbstractButton *button);
    [[nodiscard]] bool clickButton(QQuickAbstractButton *button);
    [[nodiscard]] bool doubleClickButton(QQuickAbstractButton *button);
    [[nodiscard]] QString visualFocusFailureMessage(QQuickControl *control);

    class ComponentCreator : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_MOC_INCLUDE(<QtQml/qqmlcomponent.h>)

    public:
        Q_INVOKABLE QQmlComponent *createComponent(const QByteArray &data);
    };

    class StyleInfo : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString styleName READ styleName CONSTANT FINAL)
        QML_ELEMENT
        QML_SINGLETON

    public:
        QString styleName() const;
    };

    class MockPlatformTheme : public QPlatformTheme
    {
        Qt::ColorScheme colorScheme() const override
        {
            return m_colorScheme;
        }
        void requestColorScheme(Qt::ColorScheme theme) override
        {
            m_colorScheme = theme;
            QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
        }

    private:
        Qt::ColorScheme m_colorScheme = QGuiApplication::styleHints()->colorScheme();
    };

    class SystemEnvironment : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        Q_INVOKABLE QString value(const QString &name);
        Q_INVOKABLE bool setValue(const QString &name, const QString &value);
    };

    [[nodiscard]] bool arePopupWindowsSupported();
    [[nodiscard]] QQuickPopup *popupParent(QQuickItem *item);
}

namespace QQuickTest
{
namespace Private {
// Overload of the one in quicktest.h.
[[nodiscard]] QByteArray qActiveFocusFailureMessage(QQuickPopup *popup);
} // namespace Private
} // namespace QQuickTest

#define VERIFY_VISUAL_FOCUS(control) \
do { \
    QVERIFY2(control->hasVisualFocus(), qUtf8Printable(visualFocusFailureMessage(control))); \
} while (false)

#define TRY_VERIFY_POPUP_OPENED(popup) \
do { \
    QTRY_VERIFY(popup->isOpened()); \
    if (auto *popupWindow = QQuickPopupPrivate::get(popup)->popupWindow) { \
        QVERIFY(QTest::qWaitForWindowExposed(popupWindow)); \
        if (QQuickTest::qIsPolishScheduled(popupWindow)) \
            QQuickTest::qWaitForPolish(popupWindow); \
    } \
} while (false)

QT_END_NAMESPACE

#endif // CONTROLSTESTUTILS_P_H

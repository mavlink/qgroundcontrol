#include "UomRegistrationUITest.h"

#include <memory>

#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MockLink.h"
#include "Vehicle.h"

UT_REGISTER_TEST(UomRegistrationUITest, TestLabel::Integration)

void UomRegistrationUITest::_setupAndToolbarRoutes()
{
    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink>, Vehicle*) {
                        navigateToConfigureView();
                        if (QTest::currentTestFailed()) {
                            return;
                        }
                        QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_uomButton")));
                        QTRY_VERIFY(findItem(_rootItem, QStringLiteral("uomRegistrationPage")));
                        verifyPanelContentVisible(QStringLiteral("UOM registration"));
                        QVERIFY(findItem(_rootItem, QStringLiteral("uomRegistrationPanel")));
                        for (const auto* name : {"uomVerifyButton", "uomActivateButton", "uomDeactivateButton"}) {
                            const auto* button = findItem(_rootItem, QString::fromLatin1(name));
                            QVERIFY(button);
                            QVERIFY(!button->isEnabled());
                        }
                        // The toolbar button calls the same main-window route; exercise that route
                        // after leaving the UOM page to catch loader/navigation regressions.
                        QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_summary")));
                        QVERIFY(QMetaObject::invokeMethod(_window, "showUomRegistrationPage"));
                        QTRY_VERIFY(findItem(_rootItem, QStringLiteral("uomRegistrationPage")));
                        verifyPanelContentVisible(QStringLiteral("UOM registration toolbar route"));
                    });
}

void UomRegistrationUITest::_toolbarStateEntry()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink>, Vehicle*) {
            auto* indicator = findItem(_rootItem, QStringLiteral("toolbar_uomRegistrationIndicator"));
            QVERIFY(indicator);
            QQmlComponent component(_engine);
            component.setData(R"qml(
            import QtQml
            QtObject {
                property bool available: false
                property string registrationText: "Registered"
                property string deviceActivationText: "Device not activated"
                property string reportText: "No acknowledgement"
            }
        )qml",
                              QUrl());
            QVERIFY2(component.isReady(), qPrintable(component.errorString()));
            std::unique_ptr<QObject> manager(component.create());
            QVERIFY(manager);
            QVERIFY(indicator->setProperty("manager", QVariant::fromValue(manager.get())));
            QTRY_VERIFY(!indicator->isVisible());
            QVERIFY(manager->setProperty("available", true));
            QTRY_VERIFY(indicator->isVisible());
            QVERIFY(clickButton(QStringLiteral("toolbar_uomRegistrationIndicator")));
            for (const auto& entry : {std::pair{"toolbar_uomRegistrationState", "Registration: Registered"},
                                      std::pair{"toolbar_uomDeviceState", "Device: Device not activated"},
                                      std::pair{"toolbar_uomReportState", "UOM acknowledgement: No acknowledgement"}}) {
                auto* label = findVisibleItem(_rootItem, QString::fromLatin1(entry.first));
                QVERIFY(label);
                QCOMPARE(label->property("text").toString(), QString::fromLatin1(entry.second));
            }
            QVERIFY(clickButton(QStringLiteral("toolbar_uomRegistrationButton")));
            QTRY_VERIFY(findItem(_rootItem, QStringLiteral("uomRegistrationPage")));
            QVERIFY(indicator->setProperty("manager", QVariant::fromValue<QObject*>(nullptr)));
        });
}

void UomRegistrationUITest::_uomConfigurationAndCancellation()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [&](QPointer<MockLink>, Vehicle*) {
            navigateToConfigureView();
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_uomButton")));
            auto* panel = findItem(_rootItem, QStringLiteral("uomRegistrationPanel"));
            QVERIFY(panel);

            QQmlComponent component(_engine);
            component.setData(R"qml(
            import QtQml
            QtObject {
                property string productModel: "Test aircraft"
                property string environment: "integration"
                property string producerId: "aaaaaaaa-1111-2222-3333-bbbbbbbbbbbb"
                property string expectedSerial: "TEST0000000000000001"
                property string serial: expectedSerial
                property string uid: "test-rid-uid"
                property string verifyUrl: "https://uom.caac.gov.cn/test/verify"
                property string reportUrl: "https://uom.caac.gov.cn/test/report"
                property string cancelUrl: "https://uom.caac.gov.cn/test/cancel"
                property string registrationText: "Registered"
                property string deviceActivationText: "Device not activated"
                property string reportText: "No acknowledgement"
                property string lastVerifiedAt: "2026-10-27T10:00:00Z"
                property string lastReportedAt: ""
                property string statusText: "Ready"
                property string credentialStatus: "Session-only credentials"
                property string safetyStatus: "Read-only safety status"
                property bool available: true
                property bool pending: false
                property bool canConfigure: true
                property bool canVerify: !pending
                property bool canActivate: !pending
                property bool canDeactivate: !pending
                property bool canRetry: pending
                property int cancellationCount: 0
                property string cancellationType: ""
                property string cancellationReason: ""
                function configurationForEnvironment(environment) {
                    return { producerId: "saved-" + environment, expectedSerial: "TEST0000000000000002",
                             verifyUrl: "https://uom.caac.gov.cn/" + environment + "/verify",
                             reportUrl: "https://uom.caac.gov.cn/" + environment + "/report",
                             cancelUrl: "https://uom.caac.gov.cn/" + environment + "/cancel" }
                }
                function deactivateAndCancel(type, reason) {
                    cancellationType = type
                    cancellationReason = reason
                    cancellationCount++
                }
            }
        )qml",
                              QUrl());
            QVERIFY2(component.isReady(), qPrintable(component.errorString()));
            std::unique_ptr<QObject> manager(component.create());
            QVERIFY(manager);
            QVERIFY(panel->setProperty("manager", QVariant::fromValue(manager.get())));
            QCOMPARE(findItem(_rootItem, QStringLiteral("uomRegistrationState"))->property("text").toString(),
                     QStringLiteral("Registered"));
            QCOMPARE(findItem(_rootItem, QStringLiteral("uomDeviceActivationState"))->property("text").toString(),
                     QStringLiteral("Device not activated"));
            QCOMPARE(findItem(_rootItem, QStringLiteral("uomReportState"))->property("text").toString(),
                     QStringLiteral("No acknowledgement"));

            QVERIFY(
                clickButtonScrolled(QStringLiteral("uomConfigureButton"), QStringLiteral("uomRegistrationFlickable")));
            QVERIFY(waitForDialog(QStringLiteral("Configure UOM connection")));
            auto* environment = findItem(_rootItem, QStringLiteral("uomEnvironmentInput"));
            auto* firstKeyInput = findItem(_rootItem, QStringLiteral("uomKeyInput"));
            QVERIFY(environment);
            QVERIFY(firstKeyInput);
            QVERIFY(firstKeyInput->setProperty("text", QStringLiteral("test-input-not-a-key")));
            QVERIFY(environment->setProperty("currentIndex", 1));
            QVERIFY(QMetaObject::invokeMethod(environment, "activated", Q_ARG(int, 1)));
            QVERIFY(verifyText(QStringLiteral("uomProducerInput"), QStringLiteral("saved-official"),
                               QStringLiteral("load selected environment")));
            QVERIFY(verifyText(QStringLiteral("uomVerifyUrlInput"),
                               QStringLiteral("https://uom.caac.gov.cn/official/verify"),
                               QStringLiteral("load official endpoint")));
            QVERIFY(firstKeyInput->property("text").toString().isEmpty());
            QVERIFY(rejectDialog());

            QVERIFY(manager->setProperty("pending", true));
            QVERIFY(
                clickButtonScrolled(QStringLiteral("uomConfigureButton"), QStringLiteral("uomRegistrationFlickable")));
            QVERIFY(waitForDialog(QStringLiteral("Configure UOM connection")));
            for (const auto* name : {"uomEnvironmentInput", "uomExpectedSerialInput"}) {
                QVERIFY(verifyEnabled(QString::fromLatin1(name), false, QStringLiteral("pending profile locked")));
            }
            for (const auto* name :
                 {"uomProducerInput", "uomVerifyUrlInput", "uomReportUrlInput", "uomCancelUrlInput"}) {
                QVERIFY(
                    verifyEnabled(QString::fromLatin1(name), true, QStringLiteral("pending profile can be repaired")));
            }
            auto* producerInput = findItem(_rootItem, QStringLiteral("uomProducerInput"));
            QVERIFY(producerInput);
            QVERIFY(producerInput->setProperty("text", QStringLiteral("corrected-producer")));
            QVERIFY(verifyEnabled(QStringLiteral("popupDialog_acceptButton"), false,
                                  QStringLiteral("profile repair needs explicit confirmation")));
            auto* repairConfirmation = findItem(_rootItem, QStringLiteral("uomProfileRepairConfirmation"));
            QVERIFY(repairConfirmation);
            QVERIFY(repairConfirmation->isVisible());
            QVERIFY(repairConfirmation->setProperty("checked", true));
            QVERIFY(verifyEnabled(QStringLiteral("popupDialog_acceptButton"), true,
                                  QStringLiteral("confirmed profile repair can be saved")));
            auto* keyInput = findItem(_rootItem, QStringLiteral("uomKeyInput"));
            QVERIFY(keyInput);
            QVERIFY(keyInput->isEnabled());
            QCOMPARE(keyInput->property("echoMode").toInt(), 2);  // TextInput.Password
            QVERIFY(keyInput->property("text").toString().isEmpty());
            QVERIFY(rejectDialog());
            QVERIFY(clickButtonScrolled(QStringLiteral("uomRetryButton"), QStringLiteral("uomRegistrationFlickable")));
            QVERIFY(waitForDialog(QStringLiteral("Retry pending operation")));
            QVERIFY(manager->setProperty("uid", QStringLiteral("replacement-rid-uid")));
            QVERIFY(verifyEnabled(QStringLiteral("popupDialog_acceptButton"), false,
                                  QStringLiteral("retry remains bound to the original RID")));
            QVERIFY(rejectDialog());
            QVERIFY(manager->setProperty("uid", QStringLiteral("test-rid-uid")));

            QVERIFY(manager->setProperty("pending", false));
            QVERIFY(
                clickButtonScrolled(QStringLiteral("uomDeactivateButton"), QStringLiteral("uomRegistrationFlickable")));
            QVERIFY(waitForDialog(QStringLiteral("Deactivate and cancel registration")));
            auto* reasonType = findItem(_rootItem, QStringLiteral("uomCancellationTypeInput"));
            auto* reason = findItem(_rootItem, QStringLiteral("uomCancellationReasonInput"));
            auto* confirmed = findItem(_rootItem, QStringLiteral("uomCancellationConfirmed"));
            QVERIFY(reasonType);
            QVERIFY(reason);
            QVERIFY(confirmed);
            QVERIFY(reasonType->setProperty("currentIndex", 1));
            QVERIFY(reason->setProperty("text", QString(67, QChar(0x4e2d))));
            QVERIFY(confirmed->setProperty("checked", true));
            QVERIFY(verifyEnabled(QStringLiteral("popupDialog_acceptButton"), false,
                                  QStringLiteral("201-byte reason rejected")));
            QVERIFY(reason->setProperty("text", QStringLiteral("Ownership transfer")));
            QVERIFY(confirmed->setProperty("checked", false));
            QVERIFY(verifyEnabled(QStringLiteral("popupDialog_acceptButton"), false,
                                  QStringLiteral("explicit confirmation required")));
            QCOMPARE(manager->property("cancellationCount").toInt(), 0);
            QVERIFY(confirmed->setProperty("checked", true));
            QVERIFY(acceptDialog());
            QCOMPARE(manager->property("cancellationCount").toInt(), 1);
            QCOMPARE(manager->property("cancellationType").toString(), QStringLiteral("3"));
            QCOMPARE(manager->property("cancellationReason").toString(), QStringLiteral("Ownership transfer"));
            for (const int index : {0, 2}) {
                QVERIFY(clickButtonScrolled(QStringLiteral("uomDeactivateButton"),
                                            QStringLiteral("uomRegistrationFlickable")));
                QVERIFY(waitForDialog(QStringLiteral("Deactivate and cancel registration")));
                auto* typeInput = findItem(_rootItem, QStringLiteral("uomCancellationTypeInput"));
                auto* reasonInput = findItem(_rootItem, QStringLiteral("uomCancellationReasonInput"));
                auto* confirmation = findItem(_rootItem, QStringLiteral("uomCancellationConfirmed"));
                QVERIFY(typeInput);
                QVERIFY(reasonInput);
                QVERIFY(confirmation);
                QVERIFY(typeInput->setProperty("currentIndex", index));
                QVERIFY(reasonInput->setProperty("text", QStringLiteral("Device retired")));
                QVERIFY(confirmation->setProperty("checked", true));
                QVERIFY(acceptDialog());
                QCOMPARE(manager->property("cancellationType").toString(),
                         index == 0 ? QStringLiteral("0") : QStringLiteral("4"));
            }
            QVERIFY(panel->setProperty("manager", QVariant::fromValue<QObject*>(nullptr)));
        });
}

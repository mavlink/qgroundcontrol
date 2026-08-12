/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Authentication ViewModel
 *
 * ViewModel exsposing Security & Session state to QML Presentation Layer
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <memory>

namespace Volador {

class VoladorSecurityService;

/**
 * @brief VoladorAuthViewModel connects the underlying VoladorSecurityService to QML UI components.
 */
class VoladorAuthViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY authenticationChanged)
    Q_PROPERTY(QString currentUser READ currentUser NOTIFY authenticationChanged)
    Q_PROPERTY(QString userRole READ userRole NOTIFY authenticationChanged)
    Q_PROPERTY(QString fullName READ fullName NOTIFY authenticationChanged)
    Q_PROPERTY(QString loginError READ loginError NOTIFY loginErrorChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)

public:
    explicit VoladorAuthViewModel(VoladorSecurityService *securityService, QObject *parent = nullptr);
    ~VoladorAuthViewModel() override = default;

    bool isAuthenticated() const;
    QString currentUser() const;
    QString userRole() const;
    QString fullName() const;
    QString loginError() const { return _loginError; }
    bool isBusy() const { return _isBusy; }

    Q_INVOKABLE bool login(const QString &username, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void clearError();

signals:
    void authenticationChanged();
    void loginErrorChanged(const QString &error);
    void busyChanged(bool busy);
    void loginSuccess();

private:
    VoladorSecurityService *_securityService{nullptr};
    QString _loginError;
    bool _isBusy{false};
};

} // namespace Volador

/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Authentication ViewModel
 *
 * ViewModel Implementation
 *
 ****************************************************************************/

#include "VoladorAuthViewModel.h"
#include "../Security/VoladorSecurityService.h"

#include <QtCore/QDebug>
#include <QtConcurrent/QtConcurrent>

namespace Volador {

VoladorAuthViewModel::VoladorAuthViewModel(VoladorSecurityService *securityService, QObject *parent)
    : QObject(parent)
    , _securityService(securityService)
{
    if (_securityService) {
        connect(_securityService, &VoladorSecurityService::authenticationStateChanged,
                this, [this]() {
            emit authenticationChanged();
        });

        connect(_securityService, &VoladorSecurityService::authenticationFailed,
                this, [this](const QString &reason) {
            _loginError = reason;
            _isBusy = false;
            emit loginErrorChanged(_loginError);
            emit busyChanged(_isBusy);
        });
    }
}

bool VoladorAuthViewModel::isAuthenticated() const {
    return _securityService ? _securityService->isAuthenticated() : false;
}

QString VoladorAuthViewModel::currentUser() const {
    return _securityService ? _securityService->activeSession().username : QString();
}

QString VoladorAuthViewModel::userRole() const {
    return _securityService ? VoladorSecurityService::roleToString(_securityService->activeSession().role) : QString();
}

QString VoladorAuthViewModel::fullName() const {
    return _securityService ? _securityService->activeSession().fullName : QString();
}

bool VoladorAuthViewModel::login(const QString &username, const QString &password) {
    if (!_securityService) {
        _loginError = QStringLiteral("Security service unavailable.");
        emit loginErrorChanged(_loginError);
        return false;
    }

    _isBusy = true;
    _loginError.clear();
    emit busyChanged(_isBusy);
    emit loginErrorChanged(_loginError);

    // Synchronous authentication check for robust execution
    bool result = _securityService->authenticate(username, password);
    _isBusy = false;
    emit busyChanged(_isBusy);

    if (result) {
        emit loginSuccess();
    }
    return result;
}

void VoladorAuthViewModel::logout() {
    if (_securityService) {
        _securityService->logout();
    }
}

void VoladorAuthViewModel::clearError() {
    if (!_loginError.isEmpty()) {
        _loginError.clear();
        emit loginErrorChanged(_loginError);
    }
}

} // namespace Volador

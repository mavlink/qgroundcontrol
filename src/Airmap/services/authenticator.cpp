#include <Airmap/services/authenticator.h>

std::shared_ptr<airmap::services::Authenticator> airmap::services::Authenticator::create(
    const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Authenticator>{new Authenticator{dispatcher, client}};
}

airmap::services::Authenticator::Authenticator(const std::shared_ptr<Dispatcher>& dispatcher,
                                         const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Authenticator::authenticate_with_password(const AuthenticateWithPassword::Params& parameters,
                                                           const AuthenticateWithPassword::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->authenticator().authenticate_with_password(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Authenticator::authenticate_anonymously(const AuthenticateAnonymously::Params& parameters,
                                                         const AuthenticateAnonymously::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->authenticator().authenticate_anonymously(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Authenticator::renew_authentication(const RenewAuthentication::Params& parameters,
                                                     const RenewAuthentication::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->authenticator().renew_authentication(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

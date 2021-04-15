<<<<<<< HEAD
#include <Airmap/services/authenticator.h>

std::shared_ptr<airmap::services::Authenticator> airmap::services::Authenticator::create(
=======
#include <Airmap/qt/authenticator.h>

std::shared_ptr<airmap::qt::Authenticator> airmap::qt::Authenticator::create(
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
    const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Authenticator>{new Authenticator{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Authenticator::Authenticator(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Authenticator::Authenticator(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                         const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::Authenticator::authenticate_with_password(const AuthenticateWithPassword::Params& parameters,
=======
void airmap::qt::Authenticator::authenticate_with_password(const AuthenticateWithPassword::Params& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                           const AuthenticateWithPassword::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->authenticator().authenticate_with_password(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Authenticator::authenticate_anonymously(const AuthenticateAnonymously::Params& parameters,
=======
void airmap::qt::Authenticator::authenticate_anonymously(const AuthenticateAnonymously::Params& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                         const AuthenticateAnonymously::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->authenticator().authenticate_anonymously(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Authenticator::renew_authentication(const RenewAuthentication::Params& parameters,
=======
void airmap::qt::Authenticator::renew_authentication(const RenewAuthentication::Params& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                     const RenewAuthentication::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->authenticator().renew_authentication(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

#ifndef AIRMAP_QT_AUTHENTICATOR_H_
#define AIRMAP_QT_AUTHENTICATOR_H_

#include <airmap/authenticator.h>
#include <airmap/client.h>
#include <Airmap/services/dispatcher.h>

#include <memory>

namespace airmap {
namespace services {

class Authenticator : public airmap::Authenticator, public std::enable_shared_from_this<Authenticator> {
 public:
  static std::shared_ptr<Authenticator> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                               const std::shared_ptr<airmap::Client>& client);

  void authenticate_with_password(const AuthenticateWithPassword::Params& params,
                                  const AuthenticateWithPassword::Callback& cb) override;

  void authenticate_anonymously(const AuthenticateAnonymously::Params& params,
                                const AuthenticateAnonymously::Callback& cb) override;

  void renew_authentication(const RenewAuthentication::Params& params,
                            const RenewAuthentication::Callback& cb) override;

 private:
  explicit Authenticator(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_AUTHENTICATOR_H_

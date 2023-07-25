#ifndef AIRMAP_QT_TELEMETRY_H_
#define AIRMAP_QT_TELEMETRY_H_

#include <airmap/client.h>
#include <Airmap/services/dispatcher.h>
#include <airmap/telemetry.h>

#include <memory>

namespace airmap {
namespace services {

class Telemetry : public airmap::Telemetry, public std::enable_shared_from_this<Telemetry> {
 public:
  static std::shared_ptr<Telemetry> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                           const std::shared_ptr<airmap::Client>& client);

  void submit_updates(const Flight& flight, const std::string& key,
                      const std::initializer_list<Update>& updates) override;

 private:
  explicit Telemetry(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_TELEMETRY_H_

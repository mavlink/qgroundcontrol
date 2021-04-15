#ifndef AIRMAP_QT_TELEMETRY_H_
#define AIRMAP_QT_TELEMETRY_H_

#include <airmap/client.h>
<<<<<<< HEAD
#include <Airmap/services/dispatcher.h>
=======
#include <Airmap/qt/dispatcher.h>
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
#include <airmap/telemetry.h>

#include <memory>

namespace airmap {
<<<<<<< HEAD
namespace services {
=======
namespace qt {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

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

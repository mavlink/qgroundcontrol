#ifndef AIRMAP_QT_AIRCRAFTS_H_
#define AIRMAP_QT_AIRCRAFTS_H_

#include <airmap/aircrafts.h>
#include <airmap/client.h>
<<<<<<< HEAD
#include <Airmap/services/dispatcher.h>
=======
#include <Airmap/qt/dispatcher.h>
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

#include <memory>
#include <string>

namespace airmap {
<<<<<<< HEAD
namespace services {
=======
namespace qt {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

class Aircrafts : public airmap::Aircrafts, public std::enable_shared_from_this<Aircrafts> {
 public:
  static std::shared_ptr<Aircrafts> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                           const std::shared_ptr<airmap::Client>& client);

  void manufacturers(const Manufacturers::Parameters& parameters, const Manufacturers::Callback& cb) override;
  void models(const Models::Parameters& parameters, const Models::Callback& cb) override;
  void model_for_id(const ModelForId::Parameters& parameters, const ModelForId::Callback& cb) override;

 private:
  explicit Aircrafts(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);
  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_AIRCRAFTS_H_

#ifndef AIRMAP_QT_AIRSPACES_H_
#define AIRMAP_QT_AIRSPACES_H_

#include <airmap/airspaces.h>
#include <airmap/client.h>
<<<<<<< HEAD
#include <Airmap/services/dispatcher.h>
=======
#include <Airmap/qt/dispatcher.h>
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

#include <memory>

namespace airmap {
<<<<<<< HEAD
namespace services {
=======
namespace qt {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

class Airspaces : public airmap::Airspaces, public std::enable_shared_from_this<Airspaces> {
 public:
  static std::shared_ptr<Airspaces> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                           const std::shared_ptr<airmap::Client>& client);

  void search(const Search::Parameters& parameters, const Search::Callback& cb) override;
  void for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) override;

 private:
  explicit Airspaces(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_AIRSPACES_H_

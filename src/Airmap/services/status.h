#ifndef AIRMAP_QT_STATUS_H_
#define AIRMAP_QT_STATUS_H_

#include <airmap/client.h>
#include <Airmap/services/dispatcher.h>
#include <airmap/status.h>

#include <memory>

namespace airmap {
namespace services {

class Status : public airmap::Status, public std::enable_shared_from_this<Status> {
 public:
  static std::shared_ptr<Status> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                        const std::shared_ptr<airmap::Client>& client);

  void get_status_by_point(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) override;
  void get_status_by_path(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) override;
  void get_status_by_polygon(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) override;

 private:
  explicit Status(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_STATUS_H_

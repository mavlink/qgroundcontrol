#ifndef AIRMAP_QT_ADVISORY_H_
#define AIRMAP_QT_ADVISORY_H_

#include <airmap/advisory.h>
#include <airmap/client.h>
#include "dispatcher.h"

namespace airmap {
namespace services {

class Advisory : public airmap::Advisory, public std::enable_shared_from_this<Advisory> {
 public:
  static std::shared_ptr<Advisory> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                          const std::shared_ptr<airmap::Client>& client);

  void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) override;
  void search(const Search::Parameters& parameters, const Search::Callback& cb) override;
  void report_weather(const ReportWeather::Parameters& parameters, const ReportWeather::Callback& cb) override;

 private:
  explicit Advisory(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);
  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_ADVISORY_H_

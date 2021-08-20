#ifndef AIRMAP_QT_TRAFFIC_H_
#define AIRMAP_QT_TRAFFIC_H_

#include <airmap/traffic.h>

#include <Airmap/services/dispatcher.h>

#include <memory>
#include <set>

namespace airmap {
namespace services {

class Traffic : public airmap::Traffic, public std::enable_shared_from_this<Traffic> {
 public:
  static std::shared_ptr<Traffic> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                         const std::shared_ptr<airmap::Client>& client);

  class Monitor : public airmap::Traffic::Monitor,
                  public airmap::Traffic::Monitor::Subscriber,
                  public std::enable_shared_from_this<Monitor> {
   public:
    static std::shared_ptr<Monitor> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                           const std::shared_ptr<airmap::Traffic::Monitor>& native);

    // From airmap::Traffic::Monitor
    void subscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) override;
    void unsubscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) override;
    // From airmap::Traffic::Monitor::Subscriber
    void handle_update(Update::Type type, const std::vector<Update>& update) override;

   private:
    explicit Monitor(const std::shared_ptr<Dispatcher>& dispatcher,
                     const std::shared_ptr<airmap::Traffic::Monitor>& native);

    std::shared_ptr<Dispatcher> dispatcher_;
    std::shared_ptr<airmap::Traffic::Monitor> native_;
    std::set<std::shared_ptr<airmap::Traffic::Monitor::Subscriber>> subscribers_;
  };

  // From airmap::Traffic
  void monitor(const Monitor::Params& params, const Monitor::Callback& cb) override;

 private:
  explicit Traffic(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_TRAFFIC_H_

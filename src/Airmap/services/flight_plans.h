#ifndef AIRMAP_QT_FLIGHT_PLANS_H_
#define AIRMAP_QT_FLIGHT_PLANS_H_

#include <airmap/client.h>
#include <airmap/flight_plans.h>
#include <Airmap/services/dispatcher.h>

#include <memory>

namespace airmap {
namespace services {

/// FlightPlans provides functionality for managing flight plans.
class FlightPlans : public airmap::FlightPlans, public std::enable_shared_from_this<FlightPlans> {
 public:
  static std::shared_ptr<FlightPlans> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                             const std::shared_ptr<airmap::Client>& client);

  void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) override;
  void create_by_polygon(const Create::Parameters& parameters, const Create::Callback& cb) override;
  void update(const Update::Parameters& parameters, const Update::Callback& cb) override;
  void delete_(const Delete::Parameters& parameters, const Delete::Callback& cb) override;
  void render_briefing(const RenderBriefing::Parameters& parameters, const RenderBriefing::Callback& cb) override;
  void submit(const Submit::Parameters& parameters, const Submit::Callback& cb) override;

 private:
  explicit FlightPlans(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_FLIGHT_PLANS_H_

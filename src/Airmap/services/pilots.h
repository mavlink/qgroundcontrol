#ifndef AIRMAP_QT_PILOTS_H_
#define AIRMAP_QT_PILOTS_H_

#include <airmap/pilots.h>

#include <airmap/pilots.h>
#include <Airmap/services/dispatcher.h>

#include <memory>

namespace airmap {
namespace services {

class Pilots : public airmap::Pilots, public std::enable_shared_from_this<Pilots> {
 public:
  static std::shared_ptr<Pilots> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                        const std::shared_ptr<airmap::Client>& client);

  void authenticated(const Authenticated::Parameters& parameters, const Authenticated::Callback& cb) override;
  void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) override;
  void update_for_id(const UpdateForId::Parameters& parameters, const UpdateForId::Callback& cb) override;
  void start_verify_pilot_phone_for_id(const StartVerifyPilotPhoneForId::Parameters& parameters,
                                       const StartVerifyPilotPhoneForId::Callback& cb) override;
  void finish_verify_pilot_phone_for_id(const FinishVerifyPilotPhoneForId::Parameters& parameters,
                                        const FinishVerifyPilotPhoneForId::Callback& cb) override;
  void aircrafts(const Aircrafts::Parameters& parameters, const Aircrafts::Callback& cb) override;
  void add_aircraft(const AddAircraft::Parameters& parameters, const AddAircraft::Callback& cb) override;
  void delete_aircraft(const DeleteAircraft::Parameters& parameters, const DeleteAircraft::Callback& cb) override;
  void update_aircraft(const UpdateAircraft::Parameters& parameters, const UpdateAircraft::Callback& cb) override;

 private:
  explicit Pilots(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_PILOTS_H_

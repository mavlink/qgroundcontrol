#ifndef AIRMAP_QT_DISPATCHER_H_
#define AIRMAP_QT_DISPATCHER_H_

#include <airmap/context.h>

#include <QEvent>
#include <QObject>

#include <functional>
#include <memory>
namespace airmap {
namespace qt {

class Dispatcher : public QObject {
 public:
  class Event : public QEvent {
   public:
    static Type registered_type();

    explicit Event(const std::function<void()>& task);
    void dispatch();

   private:
    std::function<void()> task_;
  };

  using Task = std::function<void()>;

  class ToQt : public QObject, public std::enable_shared_from_this<ToQt> {
   public:
    static std::shared_ptr<ToQt> create();
    void dispatch(const Task& task);

   private:
    ToQt();
    // From QObject
    bool event(QEvent* event) override;
  };

  class ToNative : public std::enable_shared_from_this<ToNative> {
   public:
    static std::shared_ptr<ToNative> create(const std::shared_ptr<Context>& context);
    void dispatch(const Task& task);

   private:
    explicit ToNative(const std::shared_ptr<Context>& context);
    std::shared_ptr<Context> context_;
  };

  explicit Dispatcher(const std::shared_ptr<Context>& context);

  void dispatch_to_qt(const Task& task);
  void dispatch_to_airmap(const Task& task);

 private:
  std::shared_ptr<ToQt> to_qt_;
  std::shared_ptr<ToNative> to_native_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_DISPATCHER_H_

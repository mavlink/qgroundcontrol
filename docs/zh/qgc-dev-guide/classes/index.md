# 类层次结构（上层）

## （LinkManager, LinkInterface）连接管理器类和链接接口类

QGC中的“链接”是一种特定类型的通信管道，例如串行端口或WiFi上的UDP。 LinkInterface为所有链接的基类。 每个链接都在它自己的线程上运行，并且发送字节（bytes）到MAVLink协议中。

`LinkManager` 对象将跟踪系统中所有打开的链接。 `LinkManager`也通过串行链接和 UDP链接管理自动连接。

## （MAVLinkProtocol）MAVLink协议类

系统中有一个单一的 `MAVLinkProtocol` 对象。 它的工作是接收从链路传入的字节，并将其转换为 MAVLink 信息。 MAVLink HEARTBEAT 消息已路由到 `MultivehicleManager` 。 （Vehicle）载具类

## （MultiVehicleManager）多载具管理类

系统内有一个单独的 `MultiVoilleManager` 对象。 当它在以前没有连接过的链接上收到 HEARTTBEAT（心跳包） 时，它会产生一个载具对象（object）。 `MultiVehicleManager` 还会跟踪系统中的所有载具，并处理从一个活动载具到另一个活动载具的切换，并正确处理被移除的载具。

## (Vehicle)载具类

Vehicle类所生成的对象是QGC代码与物理载具通信的主要接口。

注意：还有一个与每个Vehicle相关联的UAS对象，这是一个已弃用的类，并且正逐渐被逐步淘汰，所有功能都将转移到Vehicle类。  在此处不应添加新的代码。

## （FirmwarePlugin，FirmwarePluginManager）固件插件类和固件插件管理器类

FirmwarePlugin 类为固件插件的基类。 FirmwarePlugin类为固件插件的基类。 固件插件包含固件特定代码，因此Vehicle对象相对于它是识别的，支持UI的单个标准接口。

FirmwarePluginManager是一个工厂类，它根据Vehicle类的成员MAV_AUTOPILOT / MAV_TYPE组合创建FirmwarePlugin类的实例。

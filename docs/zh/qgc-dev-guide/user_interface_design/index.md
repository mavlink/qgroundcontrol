# 用户界面设计

QGC中UI设计的主要模式是用QML编写的UI页面，多次与用C ++编写的自定义“Controller”进行通信。 这种设计模式有点沿用MVC设计模式，但也有显著不同之处。 This follows a somewhat hacked variant of the MVC design pattern.

QML代码通过以下机制绑定到与系统关联的信息：

- 自定义控制器
- 全局QGroundControl对象，提供对活动Vehicle等内容的访问
- FactSystem提供对参数的访问，在某些情况下提供自定义事实。

注意：由于QGC中使用的QML的复杂性以及它依赖于与C ++对象的通信来驱动ui，因此无法使用Qt提供的QML Designer来编辑QML。

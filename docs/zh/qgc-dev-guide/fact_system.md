# 事实系统（Fact System）

事实系统提供一组标准化和简化QGC用户界面创建的功能。

## Fact {#fact}

事实代表系统中的单个值。

## FactMetaData

与每个事实有FactMetaData相关联 它提供有关事实的详细信息，以便驱动自动用户界面生成和验证。 它提供了关于事实的详细资料，以便驱动用户界面自动生成和验证。

## Fact Controls

事实控件是一个QML用户界面控件，它连接到Fact和它的FactMetaData，为用户提供控件以修改/显示与Fact相关的值。

## FactGroup

A _Fact Group_ is a group of [Facts](#fact).
It is used to organise facts and manage user defined facts.

## 自定义构建支持

用户定义的事实可以通过覆盖 `FirmwarePlugin` 的 `factGroups` 函数添加到自定义固件插件类中。
这些函数返回一个名为FactGroup的地图，用于识别添加的事实组。
可以通过扩展 `FactGroup` 类添加一个自定义的事实组。
FactMetaData可以使用合适的 `FactGroup` 构造函数通过提供包含必要信息的 json 文件来定义。

还可以通过覆盖 `FirmwarePlugin` 类的 `adata` 来更改现有事实的元数据。

与飞行器相关联的事实（包括属于飞行器固件插件 `factGroups` 函数中返回的事实组的事实），可通过 `getFact("factName")` 或 `getFact("factGroupName.factName")` 来获取

欲了解更多信息，请参阅 [FirmwarePlugin.h](https://github.com/mavlink/qgroundcontrol/blob/v4.0.8/src/FirmwarePlugin/FirmwarePlugin.h)中的内容。

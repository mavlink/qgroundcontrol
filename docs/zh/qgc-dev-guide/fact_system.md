# Fact System(事实系统)

Fact System(事实系统)提供一组标准化和简化QGC用户界面创建的功能。

## Fact {#fact}

事实代表系统中的单个值。

## FactMetaData

与每个事实有FactMetaData相关联 它提供有关事实的详细信息，以便驱动自动用户界面生成和验证。 It provides details on the Fact in order to drive automatic user interface generation and validation.

## 事实控制

事实控件是一个QML用户界面控件，它连接到Fact和它的FactMetaData，为用户提供控件以修改/显示与Fact相关的值。

## FactGroup（事实小组）

A _Fact Group_ is a group of [Facts](#fact).
It is used to organise facts and manage user defined facts.

## Custom Build Support

User defined facts can be added by overriding `factGroups` function of `FirmwarePlugin` in a custom firmware plugin class.
These functions return a name to fact group map that is used to identify added fact groups.
A custom fact group can be added by extending `FactGroup` class.
FactMetaDatas could be defined using the appopriate `FactGroup` constructor by providing a json file containing necessery information.

Changing the metadata of existing facts is also possible by overriding `adjustMetaData` of `FirmwarePlugin` class.

A fact associated with a vehicle (including facts belonging to fact groups returned in `factGroups` function of the vehicles Firmware plugin) can be reached using `getFact("factName")` or `getFact("factGroupName.factName")`

For additional information please refer to comments in [FirmwarePlugin.h](https://github.com/mavlink/qgroundcontrol/blob/v4.0.8/src/FirmwarePlugin/FirmwarePlugin.h).

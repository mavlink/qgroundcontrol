# Fact System

The Fact System provides a set of capabilities which standardizes and simplifies the creation of the QGC user interface.

## Fact {#fact}

A Fact represents a single value within the system.

## FactMetaData

There is `FactMetaData` associated with each fact. It provides details on the Fact in order to drive automatic user interface generation and validation.

## Fact Controls

A Fact Control is a QML user interface control which connects to a Fact and it's `FactMetaData` to provide a control to the user to modify/display the value associated with the Fact.

## FactGroup

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

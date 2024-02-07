# 参数文件格式

```
# Onboard parameters for Vehicle 1
#
# # Vehicle-Id Component-Id Name Value Type
1   1   ACRO_LOCKING    0   2
1   1   ACRO_PITCH_RATE 180 4
1   1   ACRO_ROLL_RATE  180 4
1   1   ADSB_ENABLE 0   2
```

以上是具有四个参数的参数文件的示例。 该文件可以包含所需数量的参数。 The file can include as many parameters as needed.

评论之前是＃。

此标头：#MAV ID COMPONENT ID PARAM NAME VALUE描述每行的格式：

- Vehicle-Id Vehicle id(载具编号）
- Component-Id（参数的组件编号）
- Name参数名称
- Value参数值
- 使用MAVLink MAV_PARAM_TYPE_ \*枚举值键入参数类型

参数文件包含单个Vehicle的参数。 它可以包含该Vehicle中多个组件的参数。 It can contain parameters for multiple components within that Vehicle.

# 参数文件格式

```
# 载具 1 的载荷参数
#
# # Vehicle-Id Component-Id Name Value Type
1   1   ACRO_LOCKING    0   2
1   1   ACRO_PITCH_RATE 180 4
1   1   ACRO_ROLL_RATE  180 4
1   1   ADSB_ENABLE 0   2
```

以上是具有四个参数的参数文件的例子。 该文件可以包含尽可能多的参数。

注释前面有一个 #\`。

此标头： `# MAV ID COMPONETID PARAMNAME VALUE` 描述了每行的格式：

- `Vehicle-Id` Vehicle id\` 载具编号
- `Component-Id` 参数的组件编号
- `Name` 参数名称
- `Value` 参数值
- `Type` 参数类型，使用 MAVLink `MAV_PARAM_TYPE_*` 枚举值

参数文件包含单个载具的参数。 它可以包含该载具上多个组件的参数。

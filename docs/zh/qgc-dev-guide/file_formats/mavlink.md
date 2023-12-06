# MAVLink 日志格式

_QGC 地面站_ 能生成纯 MAVLink 数据包日志，并支持日志回放，以便飞行任务结束后查看任务执行状态来进行数据分析。

The format is binary:

- 字节 1-8：64位无符号整数，表示时间戳，单位为微秒，起始时间为Unix纪元（UTM时间1970年1月1日0时0分0秒）
- 字节 9-271：MAVLink 数据包（数据包的最大长度为263字节，包括数据包起始标识。一般来说，数据包中的可用字节不会被实际数据全部填充，因此，数据包的实际长度会小于 263 字节 。 Includes packet start sign)

## Debugging

To check your data, open your written file in a hex editor. You should see after 8 bytes **0x55**. The first 8 bytes should also convert to a valid timestamp, so something either close to zero or around the number **1294571828792000** (which is the current Unix epoch timestamp in microseconds).

## 用于记录 MAVLink 的 C++ 例程

下面的代码段演示，如何使用 C++ 标准库中的 [C++ streams](http://www.cplusplus.com/reference/iostream/istream/) 实现日志记录。

```cpp
//write into mavlink logfile
const int len = MAVLINK_MAX_PACKET_LEN+sizeof(uint64_t);
uint8_t buf[len];
uint64_t time = getSystemTimeUsecs();
memcpy(buf, (void*)&time, sizeof(uint64_t));
mavlink_msg_to_send_buffer(buf+sizeof(uint64_t), msg);
mavlinkFile << buf << flush;
```

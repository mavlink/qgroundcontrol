# MAVLink 日志格式

_QGroundControll_ 允许您生成普通的 MAVLink 数据包日志，这些日志可以被
重播（通过 QGroundControll） 来再次观察任务进行分析。

格式为二进制：

- 字节 1-8：以微秒为单位的时间戳（自 Unix 时间起），无符号 64 位整数
- Byte 9-271: MAVLink 数据包（最大数据包长度为263 bytes ，不是所有字节都必须是实际数据，数据包可能较短。 包括数据包开始标志)

## 调试

要检查您的数据，请在十六进制编辑器中打开您的写入文件。 你应该在 8 字节之后查看 **0x55** 。 前 8 个字节也应转换为有效的时间戳，因此应为接近零或**1294571828792000**（以微秒为单位的当前 Unix 时间戳）。

## 用于记录 MAVLink 的 C++ 示例

下面的代码段演示了如何使用 C++ 标准库中的 [C++ streams](http://www.cplusplus.com/reference/iostream/istream/) 实现日志记录。

```cpp
//写入 mavlink 日志文件
const int len = MAVLINK_MAX_PACKET_LEN+sizeof(uint64_t);
uint8_t buf[len];
uint64_t time = getSystemTimeUsecs();
memcpy(buf, (void*)&time, sizeof(uint64_t));
mavlink_msg_to_send_buffer(buf+sizeof(uint64_t), msg);
mavlinkFile << buf << flush;
```
